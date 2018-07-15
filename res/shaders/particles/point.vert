#version 450

layout(location=0) in vec2 in_pos;
out vec4 vert_color;

//layout(location=0) uniform ivec2 win_size;

void main()
{
	//vec2 half_win_size = vec2(win_size) * 0.5;
	// gl_Position.xy = (in_pos - half_win_size) / half_win_size;
	gl_Position.xy = (in_pos - 0.5) * 2.0;
	gl_Position.z = 0;
	gl_Position.w = 1;
	//gl_PointSize = 2;
	vert_color = vec4(1.0, 0.0, 0.0, 1.0);
}
