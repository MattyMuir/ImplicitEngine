#shader vertex
#version 460 core
layout(location = 0) in vec2 aPos;

void main()
{
	gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
}

#shader fragment
#version 460 core
out vec4 FragColor;

void main()
{
	FragColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);
}