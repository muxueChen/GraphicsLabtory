//----------------------------------------------------------------------
// Declaration: Copyright (c), by i_dovelemon, 2018. All right reserved.
// Author: i_dovelemon[1322600812@qq.com]
// Date: 2018 / 03 / 14
// Brief: Demostrate how to use PBR Texture
// Update: 2018 / 06 / 03 -- Add support for saving .pfc and .pft file for image based lighting
//----------------------------------------------------------------------
#include "glb_pbs_texture.h"
#include "resource.h"

#define GLB_USE_PRELOAD_MAP (TRUE)  // Config to use offline precompute prefilter cubemap

class ApplicationIBLSpecular : public glb::app::ApplicationBase {
public:
    ApplicationIBLSpecular()
    : m_CubeMapRT(NULL)
    , m_CubeMap(NULL)
    , m_DiffuseCubeMapRT(NULL)
    , m_DiffuseCubeMap(NULL)
    , m_SpecularLDCubeMap(NULL)
    , m_SpecularDFGCubeMapRT(NULL)
    , m_SpecularDFGCubeMap(NULL)
    , m_ConvertERMapToCubeMapProgram(NULL)
    , m_DiffuseCubeMapProgram(NULL)
    , m_SpecularLDCubeMapProgram(NULL)
    , m_PBSProgram(NULL)
    , m_TestCubeMapProgram(NULL)
    , m_ERMap(NULL)
    , m_Sphere(NULL)
    , m_Camera(NULL)
    , m_Proj()
    , m_View()
    , m_WVPLoc(-1)
    , m_ERMapLoc(-1)
    , m_DiffuseProgram_CubeMapLoc(-1)
    , m_DiffuseProgram_FaceIndexLoc(-1)
    , m_SpecularLDProgram_CubeMapLoc(-1)
    , m_SpecularLDProgram_FaceIndexLoc(-1)
    , m_SpecularLDProgram_RoughnessLoc(-1)
    , m_PBSProgram_WVPLoc(-1)
    , m_PBSProgram_WorldLoc(-1)
    , m_PBSProgram_InvTransWorldMLoc(-1)
    , m_PBSProgram_AlbedoLoc(-1)
    , m_PBSProgram_RoughnessLoc(-1)
    , m_PBSProgram_MetallicLoc(-1)
    , m_PBSProgram_NormalLoc(-1)
    , m_PBSProgram_EyePosLoc(-1)
    , m_PBSProgram_IrradianceMapLoc(-1)
    , m_PBSProgram_PrefilterEnvMapLoc(-1)
    , m_PBSProgram_IntegrateBRDFMapLoc(-1)
    , m_TestCubeMap_WVPLoc(-1)
    , m_TestProgram_CubeMapLoc(-1)
#if GLB_USE_PRELOAD_MAP
    , m_PreLoadDiffseCubeMap(NULL)
    , m_PreLoadSpecularCubeMap(NULL)
    , m_PreLoadDFGMap(NULL)
#endif
    {
    }
    virtual~ApplicationIBLSpecular() {}

public:
    static glb::app::ApplicationBase* Create() {
        return new ApplicationIBLSpecular;
    }

public:
    bool Initialize() {
        // Create Render target
        m_CubeMapRT = render::RenderTarget::Create(1024, 1024);
        m_CubeMap = render::texture::Texture::CreateFloat16CubeTexture(1024, 1024);
        render::DrawColorBuffer drawBuffer[6] = {
            render::COLORBUF_COLOR_ATTACHMENT0,
            render::COLORBUF_COLOR_ATTACHMENT1,
            render::COLORBUF_COLOR_ATTACHMENT2,
            render::COLORBUF_COLOR_ATTACHMENT3,
            render::COLORBUF_COLOR_ATTACHMENT4,
            render::COLORBUF_COLOR_ATTACHMENT5,
        };
        m_CubeMapRT->AttachCubeTexture(drawBuffer, m_CubeMap);

        m_DiffuseCubeMapRT = render::RenderTarget::Create(32, 32);
        m_DiffuseCubeMap = render::texture::Texture::CreateFloat16CubeTexture(32, 32);
        m_DiffuseCubeMapRT->AttachCubeTexture(drawBuffer, m_DiffuseCubeMap);

#if GLB_USE_PRELOAD_MAP
        m_PreLoadDiffseCubeMap = render::texture::Texture::CreatePrefilterCubeMap("diffuse.pfc");
#endif

        m_SpecularLDCubeMap = render::texture::Texture::CreateFloat16CubeTexture(128, 128);
        int32_t width = 128, height = 128;
        for (int32_t i = 0; i < 8; i++) {
            m_SpecularLDCubeMapRT[i] = render::RenderTarget::Create(width, height);
            m_SpecularLDCubeMapRT[i]->AttachCubeTexture(drawBuffer, m_SpecularLDCubeMap, i);
            width /= 2;
            height /= 2;
        }

#if GLB_USE_PRELOAD_MAP
        m_PreLoadSpecularCubeMap = render::texture::Texture::CreatePrefilterCubeMap("specular.pfc");
#endif

        m_SpecularDFGCubeMap = render::texture::Texture::CreateFloat32Texture(128, 128);
        m_SpecularDFGCubeMapRT = render::RenderTarget::Create(128, 128);
        m_SpecularDFGCubeMapRT->AttachColorTexture(render::COLORBUF_COLOR_ATTACHMENT0, m_SpecularDFGCubeMap);

#if GLB_USE_PRELOAD_MAP
        m_PreLoadDFGMap = render::texture::Texture::CreatePrefilterTableMap("dfg.pft");
#endif

        // Create Shader
        m_ConvertERMapToCubeMapProgram = render::shader::UserProgram::Create("res/filtering_ermap.vs", "res/filtering_ermap.fs");
        m_WVPLoc = m_ConvertERMapToCubeMapProgram->GetUniformLocation("glb_WVP");
        m_ERMapLoc = m_ConvertERMapToCubeMapProgram->GetUniformLocation("glb_EquirectangularMap");

        m_DiffuseCubeMapProgram = render::shader::UserProgram::Create("res/diffuse.vs", "res/diffuse.fs");
        m_DiffuseProgram_CubeMapLoc = m_DiffuseCubeMapProgram->GetUniformLocation("glb_CubeMap");
        m_DiffuseProgram_FaceIndexLoc = m_DiffuseCubeMapProgram->GetUniformLocation("glb_FaceIndex");

        m_SpecularLDCubeMapProgram = render::shader::UserProgram::Create("res/specular_ld.vs", "res/specular_ld.fs");
        m_SpecularLDProgram_CubeMapLoc = m_SpecularLDCubeMapProgram->GetUniformLocation("glb_CubeMap");
        m_SpecularLDProgram_FaceIndexLoc = m_SpecularLDCubeMapProgram->GetUniformLocation("glb_FaceIndex");
        m_SpecularLDProgram_RoughnessLoc = m_SpecularLDCubeMapProgram->GetUniformLocation("glb_Roughness");

        m_SpecularDFGCubeMapProgram = render::shader::UserProgram::Create("res/specular_dfg.vs", "res/specular_dfg.fs");

        m_PBSProgram = render::shader::UserProgram::Create("res/pbs.vs", "res/pbs.fs");
        m_PBSProgram_WVPLoc = m_PBSProgram->GetUniformLocation("glb_WVP");
        m_PBSProgram_WorldLoc = m_PBSProgram->GetUniformLocation("glb_World");
        m_PBSProgram_InvTransWorldMLoc = m_PBSProgram->GetUniformLocation("glb_InvTransWorldM");
        m_PBSProgram_AlbedoLoc = m_PBSProgram->GetUniformLocation("glb_AlbedoMap");
        m_PBSProgram_RoughnessLoc = m_PBSProgram->GetUniformLocation("glb_RoughnessMap");
        m_PBSProgram_MetallicLoc = m_PBSProgram->GetUniformLocation("glb_MetallicMap");
        m_PBSProgram_NormalLoc = m_PBSProgram->GetUniformLocation("glb_NormalMap");
        m_PBSProgram_EyePosLoc = m_PBSProgram->GetUniformLocation("glb_EyePos");
        m_PBSProgram_IrradianceMapLoc = m_PBSProgram->GetUniformLocation("glb_IrradianceMap");
        m_PBSProgram_PrefilterEnvMapLoc = m_PBSProgram->GetUniformLocation("glb_PerfilterEnvMap");
        m_PBSProgram_IntegrateBRDFMapLoc = m_PBSProgram->GetUniformLocation("glb_IntegrateBRDFMap");

        m_TestCubeMapProgram = render::shader::UserProgram::Create("res/test_cubemap.vs", "res/test_cubemap.fs");
        m_TestCubeMap_WVPLoc = m_TestCubeMapProgram->GetUniformLocation("glb_WVP");
        m_TestProgram_CubeMapLoc = m_TestCubeMapProgram->GetUniformLocation("glb_CubeMap");

        // Create Mesh
        m_Sphere = render::mesh::Mgr::GetMeshById(render::mesh::Mgr::AddMesh("res/sphere.obj"));
        m_ScreenMesh = render::mesh::ScreenMesh::Create();

        // Create texture
        m_ERMap = render::texture::Texture::Create("res/ermap.hdr");
        m_AlbedoMap[0] = render::texture::Texture::Create("res/rustediron_basecolor.dds");
        m_RoughnessMap[0] = render::texture::Texture::Create("res/rustediron_roughness.dds");
        m_MetalicMap[0] = render::texture::Texture::Create("res/rustediron_metallic.dds");
        m_NormalMap[0] = render::texture::Texture::Create("res/rustediron_normal.dds");
        m_AlbedoMap[1] = render::texture::Texture::Create("res/streakedmarble_basecolor.dds");
        m_RoughnessMap[1] = render::texture::Texture::Create("res/streakedmarble_roughness.dds");
        m_MetalicMap[1] = render::texture::Texture::Create("res/streakedmarble_metallic.dds");
        m_NormalMap[1] = render::texture::Texture::Create("res/streakedmarble_normal.dds");
        m_AlbedoMap[2] = render::texture::Texture::Create("res/bathroomtile_basecolor.dds");
        m_RoughnessMap[2] = render::texture::Texture::Create("res/bathroomtile_roughness.dds");
        m_MetalicMap[2] = render::texture::Texture::Create("res/bathroomtile_metallic.dds");
        m_NormalMap[2] = render::texture::Texture::Create("res/bathroomtile_normal.dds");

        // Create Camera
        m_Camera = scene::ModelCamera::Create(math::Vector(0.0f, 0.0f, 320.0f), math::Vector(0.0f, 0.0f, 0.0f));

        m_Proj.MakeProjectionMatrix(1.0f * app::Application::GetWindowWidth() / app::Application::GetWindowHeight(), 60.0f, 0.01f, 10000.0f);
        m_View = m_Camera->GetViewMatrix();

        return true;
    }

    void Update(float dt) {
        util::ProfileTime time;
        time.BeginProfile();

        glb::Input::Update();

        // Calculate irradiance map
        static bool isIrradianceMapGenerated = false;
        if (!isIrradianceMapGenerated) {
            DrawCubeMap();
            // Generate mipmap for rendering
            m_CubeMap->GenerateMipmap();

            DrawConvolutionCubeMapDiffuse();
            m_DiffuseCubeMap->GenerateMipmap();
            render::Device::SetRenderTarget(render::RenderTarget::DefaultRenderTarget());

            //m_DiffuseCubeMap->SavePrefilterCubeMap("diffuse.pfc");

            DrawConvolutionCubeMapSpecularLD();
            DrawConvolutionCubeMapSpecularDFG();

            //m_SpecularDFGCubeMap->SavePrefilterTableMap("dfg.pft");
            //m_SpecularLDCubeMap->SavePrefilterCubeMap("specular.pfc");

            isIrradianceMapGenerated = true;
        }

        // Draw the cube map to check it
        //DrawTestCubeMap();

        // Draw scene
        DrawScene();

        // Reset render target
        render::Device::SetRenderTarget(render::RenderTarget::DefaultRenderTarget());

        // SwapBuffers
        render::Device::SwapBuffer();

        GLB_CHECK_GL_ERROR;

        time.EndProfile();
        printf("%f\n", time.GetProfileTimeInMs());
    }

    void Destroy() {
        GLB_SAFE_DELETE(m_CubeMapRT);
        GLB_SAFE_DELETE(m_CubeMap);
        GLB_SAFE_DELETE(m_DiffuseCubeMapRT);
        GLB_SAFE_DELETE(m_DiffuseCubeMap);
        for (int32_t i = 0; i < 8; i++) {
            GLB_SAFE_DELETE(m_SpecularLDCubeMapRT[i]);
        }
        GLB_SAFE_DELETE(m_SpecularLDCubeMap);
        GLB_SAFE_DELETE(m_SpecularDFGCubeMapRT);
        GLB_SAFE_DELETE(m_SpecularDFGCubeMap);
        GLB_SAFE_DELETE(m_ConvertERMapToCubeMapProgram);
        GLB_SAFE_DELETE(m_DiffuseCubeMapProgram);
        GLB_SAFE_DELETE(m_SpecularLDCubeMapProgram);
        GLB_SAFE_DELETE(m_SpecularDFGCubeMapProgram);
        GLB_SAFE_DELETE(m_PBSProgram);
        GLB_SAFE_DELETE(m_TestCubeMapProgram);
        GLB_SAFE_DELETE(m_ERMap);
        for (int32_t i = 0; i < 3; i++) {
            GLB_SAFE_DELETE(m_AlbedoMap[i]);
            GLB_SAFE_DELETE(m_RoughnessMap[i]);
            GLB_SAFE_DELETE(m_MetalicMap[i]);
            GLB_SAFE_DELETE(m_NormalMap[i]);
        }
        GLB_SAFE_DELETE(m_ScreenMesh);
        GLB_SAFE_DELETE(m_Camera);

#if GLB_USE_PRELOAD_MAP
        GLB_SAFE_DELETE(m_PreLoadDiffseCubeMap);
        GLB_SAFE_DELETE(m_PreLoadSpecularCubeMap);
#endif
    }

    void DrawCubeMap() {
        // New perspective matrix
        math::Matrix proj;
        proj.MakeProjectionMatrix(1.0f, 90.0f, 0.1f, 1000.0f);

        // 6 View matrix(+X,-X,+Y,-Y,+Z,-Z)
        math::Matrix views[6];
        views[0].MakeViewMatrix(math::Vector(0.0f, 0.0f, 0.0f), math::Vector(0.0f, 0.0f, 0.0f) + math::Vector(1.0f, 0.0f, 0.0f));
        views[1].MakeViewMatrix(math::Vector(0.0f, 0.0f, 0.0f), math::Vector(0.0f, 0.0f, 0.0f) + math::Vector(-1.0f, 0.0f, 0.0f));
        views[2].MakeViewMatrix(math::Vector(0.0f, 0.0f, 0.0f), math::Vector(1.0f, 0.0f, 0.0f), math::Vector(0.0f, 0.0f, 1.0f), math::Vector(0.0f, -1.0f, 0.0f));
        views[3].MakeViewMatrix(math::Vector(0.0f, 0.0f, 0.0f), math::Vector(1.0f, 0.0f, 0.0f), math::Vector(0.0f, 0.0f, -1.0f), math::Vector(0.0f, 1.0f, 0.0f));
        views[4].MakeViewMatrix(math::Vector(0.0f, 0.0f, 0.0f), math::Vector(0.0f, 0.0f, 0.0f) + math::Vector(0.0f, 0.0f, 1.0f));
        views[5].MakeViewMatrix(math::Vector(0.0f, 0.0f, 0.0f), math::Vector(0.0f, 0.0f, 0.0f) + math::Vector(0.0f, 0.0f, -1.0f));

        // Render Target
        render::Device::SetRenderTarget(m_CubeMapRT);

        // View port
        render::Device::SetViewport(0, 0, 1024, 1024);

        // Setup shader
        render::Device::SetShader(m_ConvertERMapToCubeMapProgram);
        render::Device::SetShaderLayout(m_ConvertERMapToCubeMapProgram->GetShaderLayout());

        // Setup texture
        render::Device::ClearTexture();
        render::Device::SetTexture(0, m_ERMap, 0);

        // Setup mesh
        render::Device::SetVertexLayout(m_Sphere->GetVertexLayout());
        render::Device::SetVertexBuffer(m_Sphere->GetVertexBuffer());

        // Setup render state
        render::Device::SetDepthTestEnable(true);
        render::Device::SetCullFaceEnable(true);
        render::Device::SetCullFaceMode(render::CULL_FRONT);

        for (int32_t i = 0; i < 6; i++) {
            // Set Draw Color Buffer
            render::Device::SetDrawColorBuffer(static_cast<render::DrawColorBuffer>(render::COLORBUF_COLOR_ATTACHMENT0 + i));

            // Clear
            render::Device::SetClearColor(0.0f, 0.0f, 0.0f);
            render::Device::SetClearDepth(1.0f);
            render::Device::Clear(render::CLEAR_COLOR | render::CLEAR_DEPTH);

            // Setup uniform
            math::Matrix world;
            world.MakeIdentityMatrix();

            math::Matrix wvp;
            wvp.MakeIdentityMatrix();
            wvp = proj * views[i] * world;
            math::Matrix inv_trans_world = world;
            inv_trans_world.Inverse();
            inv_trans_world.Transpose();

            render::Device::SetUniformMatrix(m_WVPLoc, wvp);
            render::Device::SetUniformSampler2D(m_ERMapLoc, 0);

            // Draw
            render::Device::Draw(render::PT_TRIANGLES, 0, m_Sphere->GetVertexNum());
        }
    }

    void DrawConvolutionCubeMapDiffuse() {
        // Render Target
        render::Device::SetRenderTarget(m_DiffuseCubeMapRT);

        // View port
        render::Device::SetViewport(0, 0, 32, 32);

        // Setup shader
        render::Device::SetShader(m_DiffuseCubeMapProgram);
        render::Device::SetShaderLayout(m_DiffuseCubeMapProgram->GetShaderLayout());

        // Setup texture
        render::Device::ClearTexture();
        render::Device::SetTexture(0, m_CubeMap, 0);

        // Setup mesh
        render::Device::SetVertexLayout(m_ScreenMesh->GetVertexLayout());
        render::Device::SetVertexBuffer(m_ScreenMesh->GetVertexBuffer());

        // Setup render state
        render::Device::SetDepthTestEnable(true);
        render::Device::SetCullFaceEnable(true);
        render::Device::SetCullFaceMode(render::CULL_BACK);

        for (int32_t i = 0; i < 6; i++) {
            // Set Draw Color Buffer
            render::Device::SetDrawColorBuffer(static_cast<render::DrawColorBuffer>(render::COLORBUF_COLOR_ATTACHMENT0 + i));

            // Clear
            render::Device::SetClearColor(0.0f, 0.0f, 0.0f);
            render::Device::SetClearDepth(1.0f);
            render::Device::Clear(render::CLEAR_COLOR | render::CLEAR_DEPTH);

            // Setup uniform
            render::Device::SetUniformSamplerCube(m_DiffuseProgram_CubeMapLoc, 0);
            render::Device::SetUniform1i(m_DiffuseProgram_FaceIndexLoc, i);

            // Draw
            render::Device::Draw(render::PT_TRIANGLES, 0, m_ScreenMesh->GetVertexNum());
        }
    }

    void DrawConvolutionCubeMapSpecularLD() {
        // Setup shader
        render::Device::SetShader(m_SpecularLDCubeMapProgram);
        render::Device::SetShaderLayout(m_SpecularLDCubeMapProgram->GetShaderLayout());

        // Setup texture
        render::Device::ClearTexture();
        render::Device::SetTexture(0, m_CubeMap, 0);

        // Setup mesh
        render::Device::SetVertexLayout(m_ScreenMesh->GetVertexLayout());
        render::Device::SetVertexBuffer(m_ScreenMesh->GetVertexBuffer());

        // Setup render state
        render::Device::SetDepthTestEnable(true);
        render::Device::SetCullFaceEnable(true);
        render::Device::SetCullFaceMode(render::CULL_BACK);

        int32_t miplevels = static_cast<int32_t>(log(128) / log(2) + 1);
        float roughnessStep = 1.0f / miplevels;
        int32_t width = 128, height = 128;
        for (int32_t j = 0; j < miplevels; j++) {
            // Render Target
            render::Device::SetRenderTarget(m_SpecularLDCubeMapRT[j]);

            // View port
            render::Device::SetViewport(0, 0, width, height);
            width /= 2;
            height /= 2;

            for (int32_t i = 0; i < 6; i++) {
                // Set Draw Color Buffer
                render::Device::SetDrawColorBuffer(static_cast<render::DrawColorBuffer>(render::COLORBUF_COLOR_ATTACHMENT0 + i));

                // Clear
                render::Device::SetClearColor(0.0f, 0.0f, 0.0f);
                render::Device::SetClearDepth(1.0f);
                render::Device::Clear(render::CLEAR_COLOR | render::CLEAR_DEPTH);

                // Setup uniform
                render::Device::SetUniformSamplerCube(m_SpecularLDProgram_CubeMapLoc, 0);
                render::Device::SetUniform1i(m_SpecularLDProgram_FaceIndexLoc, i);
                render::Device::SetUniform1f(m_SpecularLDProgram_RoughnessLoc, j * roughnessStep);

                // Draw
                render::Device::Draw(render::PT_TRIANGLES, 0, m_ScreenMesh->GetVertexNum());
            }
        }
    }

    void DrawConvolutionCubeMapSpecularDFG() {
        // Setup shader
        render::Device::SetShader(m_SpecularDFGCubeMapProgram);
        render::Device::SetShaderLayout(m_SpecularDFGCubeMapProgram->GetShaderLayout());

        // Setup texture
        render::Device::ClearTexture();

        // Setup mesh
        render::Device::SetVertexLayout(m_ScreenMesh->GetVertexLayout());
        render::Device::SetVertexBuffer(m_ScreenMesh->GetVertexBuffer());

        // Setup render state
        render::Device::SetDepthTestEnable(true);
        render::Device::SetCullFaceEnable(true);
        render::Device::SetCullFaceMode(render::CULL_BACK);

        // Render Target
        render::Device::SetRenderTarget(m_SpecularDFGCubeMapRT);

        // View port
        render::Device::SetViewport(0, 0, 128, 128);

        // Set Draw Color Buffer
        render::Device::SetDrawColorBuffer(static_cast<render::DrawColorBuffer>(render::COLORBUF_COLOR_ATTACHMENT0));

        // Clear
        render::Device::SetClearColor(0.0f, 0.0f, 0.0f);
        render::Device::SetClearDepth(1.0f);
        render::Device::Clear(render::CLEAR_COLOR | render::CLEAR_DEPTH);

        // Draw
        render::Device::Draw(render::PT_TRIANGLES, 0, m_ScreenMesh->GetVertexNum());
    }

    void DrawTestCubeMap() {
        // Set Render Target
        render::Device::SetRenderTarget(render::RenderTarget::DefaultRenderTarget());

        // Set Viewport
        render::Device::SetViewport(0, 0, app::Application::GetWindowWidth(), app::Application::GetWindowHeight());

        // Clear
        render::Device::SetClearColor(0.0f, 0.0f, 0.0f);
        render::Device::SetClearDepth(1.0f);
        render::Device::Clear(render::CLEAR_COLOR | render::CLEAR_DEPTH);

        // Setup shader
        render::Device::SetShader(m_TestCubeMapProgram);
        render::Device::SetShaderLayout(m_TestCubeMapProgram->GetShaderLayout());

        // Setup texture
        render::Device::ClearTexture();
        //render::Device::SetTexture(0, m_DiffuseCubeMap, 0);
        //render::Device::SetTexture(0, m_CubeMap, 0);
        //render::Device::SetTexture(0, m_SpecularCubeMap, 0);

        // Setup mesh
        render::Device::SetVertexLayout(m_Sphere->GetVertexLayout());
        render::Device::SetVertexBuffer(m_Sphere->GetVertexBuffer());

        // Setup render state
        render::Device::SetDepthTestEnable(true);
        render::Device::SetCullFaceEnable(true);
        render::Device::SetCullFaceMode(render::CULL_FRONT);

        // Setup uniform
        static float sRotX = 0.0f, sRotY = 0.0f;
        math::Matrix world;
        world.MakeIdentityMatrix();
        float mouseMoveX = static_cast<float>(Input::GetMouseMoveX());
        float mouseMoveY = static_cast<float>(Input::GetMouseMoveY());
        sRotX = sRotX + mouseMoveX * 0.1f;
        sRotY = sRotY + mouseMoveY * 0.1f;
        world.RotateY(sRotX);
        world.RotateX(sRotY);
        world.Translate(0.0f, 0.0f, 200.0f);

        math::Matrix wvp;
        wvp.MakeIdentityMatrix();
        wvp = m_Proj * m_View * world;
        math::Matrix inv_trans_world = world;
        inv_trans_world.Inverse();
        inv_trans_world.Transpose();

        render::Device::SetUniformMatrix(m_TestCubeMap_WVPLoc, wvp);
        render::Device::SetUniformSamplerCube(m_TestProgram_CubeMapLoc, 0);

        // Draw
        render::Device::Draw(render::PT_TRIANGLES, 0, m_Sphere->GetVertexNum());
    }

    void DrawScene() {
        // Set Render Target
        render::Device::SetRenderTarget(render::RenderTarget::DefaultRenderTarget());

        // Set Viewport
        render::Device::SetViewport(0, 0, app::Application::GetWindowWidth(), app::Application::GetWindowHeight());

        // Clear
        render::Device::SetClearColor(0.0f, 0.0f, 0.0f);
        render::Device::SetClearDepth(1.0f);
        render::Device::Clear(render::CLEAR_COLOR | render::CLEAR_DEPTH);

        math::Vector pos = m_Camera->GetPos();
        math::Vector target = m_Camera->GetTarget();
        math::Vector look = pos - target;
        math::Matrix rot;
        rot.MakeRotateYMatrix(Input::GetMouseMoveX() * 0.1f);
        look = rot * look;
        pos = target + look;
        m_Camera->SetPos(pos);
        m_Camera->Update(1.0f);
        m_View = m_Camera->GetViewMatrix();

        DrawEnv();

        DrawSphere();
    }

    void DrawEnv() {
        // Setup shader
        render::Device::SetShader(m_TestCubeMapProgram);
        render::Device::SetShaderLayout(m_TestCubeMapProgram->GetShaderLayout());

        // Setup texture
        render::Device::ClearTexture();
        render::Device::SetTexture(0, m_CubeMap, 0);
        //render::Device::SetTexture(0, m_SpecularLDCubeMap, 0);

        // Setup mesh
        render::Device::SetVertexLayout(m_Sphere->GetVertexLayout());
        render::Device::SetVertexBuffer(m_Sphere->GetVertexBuffer());

        // Setup render state
        render::Device::SetDepthTestEnable(false);
        render::Device::SetCullFaceEnable(true);
        render::Device::SetCullFaceMode(render::CULL_FRONT);

        // Setup uniform
        math::Matrix world;
        world.MakeIdentityMatrix();
        math::Vector pos = m_Camera->GetPos();
        world.Translate(pos.x, pos.y, pos.z);

        math::Matrix wvp;
        wvp.MakeIdentityMatrix();
        wvp = m_Proj * m_View * world;
        math::Matrix inv_trans_world = world;
        inv_trans_world.Inverse();
        inv_trans_world.Transpose();

        render::Device::SetUniformMatrix(m_TestCubeMap_WVPLoc, wvp);
        render::Device::SetUniformSamplerCube(m_TestProgram_CubeMapLoc, 0);

        // Draw
        render::Device::Draw(render::PT_TRIANGLES, 0, m_Sphere->GetVertexNum());
    }

    void DrawSphere() {
        // Setup shader
        render::Device::SetShader(m_PBSProgram);
        render::Device::SetShaderLayout(m_PBSProgram->GetShaderLayout());

        // Setup mesh
        render::Device::SetVertexLayout(m_Sphere->GetVertexLayout());
        render::Device::SetVertexBuffer(m_Sphere->GetVertexBuffer());

        // Setup render state
        render::Device::SetDepthTestEnable(true);
        render::Device::SetCullFaceEnable(true);
        render::Device::SetCullFaceMode(render::CULL_BACK);

        // Setup texture
        render::Device::ClearTexture();

#if GLB_USE_PRELOAD_MAP
        render::Device::SetTexture(0, m_PreLoadDiffseCubeMap, 0);
        render::Device::SetTexture(1, m_PreLoadSpecularCubeMap, 1);
        render::Device::SetTexture(2, m_PreLoadDFGMap, 2);
#else
        render::Device::SetTexture(0, m_DiffuseCubeMap, 0);
        render::Device::SetTexture(1, m_SpecularLDCubeMap, 1);
        render::Device::SetTexture(2, m_SpecularDFGCubeMap, 2);
#endif

        for (int32_t i = 0; i < 3; i++) {
            render::Device::SetTexture(3, m_AlbedoMap[i], 3);
            render::Device::SetTexture(4, m_RoughnessMap[i], 4);
            render::Device::SetTexture(5, m_MetalicMap[i], 5);
            render::Device::SetTexture(6, m_NormalMap[i], 6);

            // Setup uniform
            math::Matrix world;
            world.MakeIdentityMatrix();
            world.Translate(-150.0f + i * 150.0f, 0.0f, 0.0f);

            math::Matrix wvp;
            wvp.MakeIdentityMatrix();
            wvp = m_Proj * m_View * world;
            math::Matrix inv_trans_world = world;
            inv_trans_world.Inverse();
            inv_trans_world.Transpose();

            render::Device::SetUniformMatrix(m_PBSProgram_WVPLoc, wvp);
            render::Device::SetUniformMatrix(m_PBSProgram_WorldLoc, world);
            render::Device::SetUniformMatrix(m_PBSProgram_InvTransWorldMLoc, inv_trans_world);
            render::Device::SetUniform3f(m_PBSProgram_EyePosLoc, m_Camera->GetPos());
            render::Device::SetUniformSamplerCube(m_PBSProgram_IrradianceMapLoc, 0);
            render::Device::SetUniformSamplerCube(m_PBSProgram_PrefilterEnvMapLoc, 1);
            render::Device::SetUniformSampler2D(m_PBSProgram_IntegrateBRDFMapLoc, 2);
            render::Device::SetUniformSampler2D(m_PBSProgram_AlbedoLoc, 3);
            render::Device::SetUniformSampler2D(m_PBSProgram_RoughnessLoc, 4);
            render::Device::SetUniformSampler2D(m_PBSProgram_MetallicLoc, 5);
            render::Device::SetUniformSampler2D(m_PBSProgram_NormalLoc, 6);

            // Draw
            render::Device::Draw(render::PT_TRIANGLES, 0, m_Sphere->GetVertexNum());
        }
    }

protected:
    render::RenderTarget*           m_CubeMapRT;
    render::texture::Texture*       m_CubeMap;
    render::RenderTarget*           m_DiffuseCubeMapRT;
    render::texture::Texture*       m_DiffuseCubeMap;
    render::RenderTarget*           m_SpecularLDCubeMapRT[8];
    render::texture::Texture*       m_SpecularLDCubeMap;
    render::RenderTarget*           m_SpecularDFGCubeMapRT;
    render::texture::Texture*       m_SpecularDFGCubeMap;
    render::shader::UserProgram*    m_ConvertERMapToCubeMapProgram;
    render::shader::UserProgram*    m_DiffuseCubeMapProgram;
    render::shader::UserProgram*    m_SpecularLDCubeMapProgram;
    render::shader::UserProgram*    m_SpecularDFGCubeMapProgram;
    render::shader::UserProgram*    m_PBSProgram;
    render::shader::UserProgram*    m_TestCubeMapProgram;
    render::texture::Texture*       m_ERMap;
    render::texture::Texture*       m_AlbedoMap[3];
    render::texture::Texture*       m_RoughnessMap[3];
    render::texture::Texture*       m_MetalicMap[3];
    render::texture::Texture*       m_NormalMap[3];
    render::mesh::MeshBase*         m_Sphere;
    render::mesh::ScreenMesh*       m_ScreenMesh;
    scene::CameraBase*              m_Camera;
    math::Matrix                    m_Proj;
    math::Matrix                    m_View;

    int32_t                         m_WVPLoc;
    int32_t                         m_ERMapLoc;

    int32_t                         m_DiffuseProgram_CubeMapLoc;
    int32_t                         m_DiffuseProgram_FaceIndexLoc;

    int32_t                         m_SpecularLDProgram_CubeMapLoc;
    int32_t                         m_SpecularLDProgram_FaceIndexLoc;
    int32_t                         m_SpecularLDProgram_RoughnessLoc;

    int32_t                         m_PBSProgram_WVPLoc;
    int32_t                         m_PBSProgram_WorldLoc;
    int32_t                         m_PBSProgram_InvTransWorldMLoc;
    int32_t                         m_PBSProgram_AlbedoLoc;
    int32_t                         m_PBSProgram_RoughnessLoc;
    int32_t                         m_PBSProgram_MetallicLoc;
    int32_t                         m_PBSProgram_NormalLoc;
    int32_t                         m_PBSProgram_EyePosLoc;
    int32_t                         m_PBSProgram_IrradianceMapLoc;
    int32_t                         m_PBSProgram_PrefilterEnvMapLoc;
    int32_t                         m_PBSProgram_IntegrateBRDFMapLoc;

    int32_t                         m_TestCubeMap_WVPLoc;
    int32_t                         m_TestProgram_CubeMapLoc;

#if GLB_USE_PRELOAD_MAP
    render::texture::Texture*       m_PreLoadDiffseCubeMap;
    render::texture::Texture*       m_PreLoadSpecularCubeMap;
    render::texture::Texture*       m_PreLoadDFGMap;
#endif
};

int _stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPreInstance, LPSTR cmdLine, int nShowCmd) {
    glb::app::AppConfig config;
    memcpy(config.caption, L"glb_pbs_texture", sizeof(L"glb_pbs_texture"));
    config.screen_width = 800;
    config.screen_height = 600;
    config.shadow_map_width = 1024;
    config.shadow_map_height = 1024;
    config.decalMapWidth = 1024;
    config.decalMapHeight = 1024;
    config.icon = IDI_ICON1;
    if (!glb::app::Application::Initialize(ApplicationIBLSpecular::Create, hInstance, config)) {
        return 0;
    }

    glb::app::Application::Update();

    glb::app::Application::Destroy();

    return 0;
}