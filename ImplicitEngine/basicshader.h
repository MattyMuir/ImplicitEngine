const char* basicShader = "#shader vertex\n#version 460 core\nlayout(location = 0) in vec2 aPos;\n\nvoid main()\n{\n\tgl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);\n}\n\n#shader fragment\n#version 460 core\nout vec4 FragColor;\n\nuniform vec4 col;\n\nvoid main()\n{\n\tFragColor = col;\n}";