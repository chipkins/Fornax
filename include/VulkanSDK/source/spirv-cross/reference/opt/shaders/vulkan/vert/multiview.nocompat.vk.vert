#version 310 es
#extension GL_OVR_multiview2 : require

layout(binding = 0, std140) uniform MVPs
{
    mat4 MVP[2];
} _19;

layout(location = 0) in vec4 Position;

void main()
{
    gl_Position = _19.MVP[gl_ViewID_OVR] * Position;
}

