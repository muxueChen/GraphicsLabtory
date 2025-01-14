//--------------------------------------------------------------------
// Declaration: Copyright (c), by i_dovelemon, 2016. All right reserved.
// Author: i_dovelemon[1322600812@qq.com]
// Date: 2016 / 06 / 29
// Brief: Gauss blur horizontal pass shader
//--------------------------------------------------------------------
#version 330

in vec2 vs_texcoord;
out vec3 color;

uniform sampler2D glb_unif_BlurTex;
uniform float glb_unif_BlurTexWidth;
uniform int glb_unif_BlurRadius;
uniform float glb_unif_BlurStep;

float kGaussNum[10] = float[]
(
0.19947139242361153,
0.17603290076919906,
0.12098555593111537,
0.06475892872301567,
0.02699555370849171,
0.008764179751319668,
0.002215933715394386,
0.000436343696989634,
6.691555758028252e-05,
7.991935088595893e-06
);

void main() {
    float total = kGaussNum[0];
    for (int i = 1; i <= glb_unif_BlurRadius; i++) {
        total = total + kGaussNum[i] * 2.0;
    }

    color = texture(glb_unif_BlurTex, vs_texcoord).xyz * kGaussNum[0] / total;
    float step = glb_unif_BlurStep / glb_unif_BlurTexWidth;

    for (int i = 1; i <= glb_unif_BlurRadius; i++) {
        color += texture2D(glb_unif_BlurTex, vec2(vs_texcoord.x - i * step, vs_texcoord.y)).xyz * kGaussNum[i] / total;
        color += texture2D(glb_unif_BlurTex, vec2(vs_texcoord.x + i * step, vs_texcoord.y)).xyz * kGaussNum[i] / total;
    }
}