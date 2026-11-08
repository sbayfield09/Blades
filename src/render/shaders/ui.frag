#version 450
layout(location=0) in vec2 v_uv;
layout(location=1) in vec4 v_color;
layout(location=0) out vec4 o_color;
layout(set=0,binding=0) uniform sampler2D u_texture;
void main() {
    vec4 color=texture(u_texture,v_uv)*v_color;
    // Match the glyph coverage * colour alpha integer truncation.
    color.a=floor(color.a*255.0+0.001)/255.0;
    o_color=color;
}
