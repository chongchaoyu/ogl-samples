//**********************************
// OpenGL Render to texture blending 
// 28/05/2010 - 26/06/2010
//**********************************
// Christophe Riccio
// g.truc.creation@gmail.com
//**********************************
// G-Truc Creation
// www.g-truc.net
//**********************************

#include <glf/glf.hpp>

namespace
{
	std::string const SAMPLE_NAME = "OpenGL Render to texture blending";
	std::string const VERTEX_SHADER_SOURCE1(glf::DATA_DIRECTORY + "400/mrt.vert");
	std::string const FRAGMENT_SHADER_SOURCE1(glf::DATA_DIRECTORY + "400/mrt.frag");
	std::string const VERTEX_SHADER_SOURCE2(glf::DATA_DIRECTORY + "400/image-2d.vert");
	std::string const FRAGMENT_SHADER_SOURCE2(glf::DATA_DIRECTORY + "400/image-2d.frag");
	std::string const TEXTURE_DIFFUSE(glf::DATA_DIRECTORY + "kueken320-rgb8.tga");
	glm::ivec2 const FRAMEBUFFER_SIZE(640, 480);
	int const SAMPLE_SIZE_WIDTH = 640;
	int const SAMPLE_SIZE_HEIGHT = 480;
	int const SAMPLE_MAJOR_VERSION = 4;
	int const SAMPLE_MINOR_VERSION = 0;

	glf::window Window(glm::ivec2(SAMPLE_SIZE_WIDTH, SAMPLE_SIZE_HEIGHT));

	GLsizei const VertexCount = 4;
	GLsizeiptr const VertexSize = VertexCount * sizeof(glf::vertex_v2fv2f);
	glf::vertex_v2fv2f const VertexData[VertexCount] =
	{
		glf::vertex_v2fv2f(glm::vec2(-1.0f,-1.0f), glm::vec2(0.0f, 0.0f)),
		glf::vertex_v2fv2f(glm::vec2( 1.0f,-1.0f), glm::vec2(1.0f, 0.0f)),
		glf::vertex_v2fv2f(glm::vec2( 1.0f, 1.0f), glm::vec2(1.0f, 1.0f)),
		glf::vertex_v2fv2f(glm::vec2(-1.0f, 1.0f), glm::vec2(0.0f, 1.0f))
	};

	GLsizei const ElementCount = 6;
	GLsizeiptr const ElementSize = ElementCount * sizeof(GLushort);
	GLushort const ElementData[ElementCount] =
	{
		0, 1, 2, 
		2, 3, 0
	};

	enum texture_type
	{
		TEXTURE_RGB8,
		TEXTURE_R,
		TEXTURE_G,
		TEXTURE_B,
		TEXTURE_MAX
	};

	enum buffer_type
	{
		BUFFER_VERTEX,
		BUFFER_ELEMENT,
		BUFFER_MAX
	};

	GLuint FramebufferName = 0;
	GLuint VertexArrayName = 0;

	GLuint ProgramNameSingle = 0;
	GLuint UniformMVPSingle = 0;
	GLuint UniformDiffuseSingle = 0;

	GLuint ProgramNameMultiple = 0;
	GLuint UniformMVPMultiple = 0;
	GLuint UniformDiffuseMultiple = 0;

	GLuint BufferName[BUFFER_MAX];
	GLuint TextureName[TEXTURE_MAX];

	glm::ivec4 Viewport[TEXTURE_MAX];
}//namespace

bool initProgram()
{
	bool Validated = true;

	if(Validated)
	{
		ProgramNameMultiple = glCreateProgram();
		GLuint VertexShader = glf::createShader(GL_VERTEX_SHADER, VERTEX_SHADER_SOURCE1);
		glAttachShader(ProgramNameMultiple, VertexShader);
		glDeleteShader(VertexShader);
		GLuint FragmentShader = glf::createShader(GL_FRAGMENT_SHADER, FRAGMENT_SHADER_SOURCE1);
		glAttachShader(ProgramNameMultiple, FragmentShader);
		glDeleteShader(FragmentShader);
		glLinkProgram(ProgramNameMultiple);
		Validated = glf::checkProgram(ProgramNameMultiple);
	}

	if(Validated)
	{
		UniformMVPMultiple = glGetUniformLocation(ProgramNameMultiple, "MVP");
		UniformDiffuseMultiple = glGetUniformLocation(ProgramNameMultiple, "Diffuse");
	}

	if(Validated)
	{
		ProgramNameSingle = glCreateProgram();
		GLuint VertexShader = glf::createShader(GL_VERTEX_SHADER, VERTEX_SHADER_SOURCE2);
		glAttachShader(ProgramNameSingle, VertexShader);
		glDeleteShader(VertexShader);
		GLuint FragmentShader = glf::createShader(GL_FRAGMENT_SHADER, FRAGMENT_SHADER_SOURCE2);
		glAttachShader(ProgramNameSingle, FragmentShader);
		glDeleteShader(FragmentShader);
		glLinkProgram(ProgramNameSingle);
		Validated = glf::checkProgram(ProgramNameSingle);
	}

	if(Validated)
	{
		UniformMVPSingle = glGetUniformLocation(ProgramNameSingle, "MVP");
		UniformDiffuseSingle = glGetUniformLocation(ProgramNameSingle, "Diffuse");
	}

	return glf::checkError("initProgram");
}

bool initVertexBuffer()
{
	glGenBuffers(BUFFER_MAX, BufferName);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, BufferName[BUFFER_ELEMENT]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, ElementSize, ElementData, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ARRAY_BUFFER, BufferName[BUFFER_VERTEX]);
    glBufferData(GL_ARRAY_BUFFER, VertexSize, VertexData, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	return glf::checkError("initArrayBuffer");
}

bool initTexture2D()
{
	glActiveTexture(GL_TEXTURE0);
	glGenTextures(TEXTURE_MAX, TextureName);

	gli::image Image = gli::import_as(TEXTURE_DIFFUSE);

	// Load image
	{
		glBindTexture(GL_TEXTURE_2D, TextureName[TEXTURE_RGB8]);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_GREEN);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_BLUE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ALPHA);

		for(std::size_t Level = 0; Level < Image.levels(); ++Level)
		{
			glTexImage2D(
				GL_TEXTURE_2D, 
				GLint(Level), 
				GL_RGB,
				GLsizei(Image[Level].dimensions().x), 
				GLsizei(Image[Level].dimensions().y), 
				0,  
				GL_RGB, 
				GL_UNSIGNED_BYTE, 
				Image[Level].data());
		}
	}

	{
		glBindTexture(GL_TEXTURE_2D, TextureName[TEXTURE_R]);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_ZERO);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_ZERO);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ZERO);
	}
	{
		glBindTexture(GL_TEXTURE_2D, TextureName[TEXTURE_G]);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_ZERO);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_ZERO);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ZERO);
	}
	{
		glBindTexture(GL_TEXTURE_2D, TextureName[TEXTURE_B]);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_ZERO);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_ZERO);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ZERO);
	}

	for(int i = TEXTURE_R; i <= TEXTURE_B; ++i)
	{
		glBindTexture(GL_TEXTURE_2D, TextureName[i]);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_GREEN);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_BLUE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ALPHA);

		glTexImage2D(
			GL_TEXTURE_2D, 
			0, 
			GL_RED,
			GLsizei(Image[0].dimensions().x), 
			GLsizei(Image[0].dimensions().y), 
			0,  
			GL_RGB, 
			GL_UNSIGNED_BYTE, 
			0);
	}

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);

	return glf::checkError("initTexture2D");
}

bool initFramebuffer()
{
	glGenFramebuffers(1, &FramebufferName);
	glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);

	for(std::size_t i = TEXTURE_R; i <= TEXTURE_B; ++i)
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + GLenum(i - TEXTURE_R), TextureName[i], 0);

	GLenum DrawBuffers[3];
	DrawBuffers[0] = GL_COLOR_ATTACHMENT0;
	DrawBuffers[1] = GL_COLOR_ATTACHMENT1;
	DrawBuffers[2] = GL_COLOR_ATTACHMENT2;

	glDrawBuffers(3, DrawBuffers);

	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		return false;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return true;
}

bool initVertexArray()
{
	glGenVertexArrays(1, &VertexArrayName);
    glBindVertexArray(VertexArrayName);
		glBindBuffer(GL_ARRAY_BUFFER, BufferName[BUFFER_VERTEX]);
		glVertexAttribPointer(glf::semantic::attr::POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(glf::vertex_v2fv2f), GLF_BUFFER_OFFSET(0));
		glVertexAttribPointer(glf::semantic::attr::TEXCOORD, 2, GL_FLOAT, GL_FALSE, sizeof(glf::vertex_v2fv2f), GLF_BUFFER_OFFSET(sizeof(glm::vec2)));
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glEnableVertexAttribArray(glf::semantic::attr::POSITION);
		glEnableVertexAttribArray(glf::semantic::attr::TEXCOORD);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, BufferName[BUFFER_ELEMENT]);
	glBindVertexArray(0);

	return glf::checkError("initVertexArray");
}

bool initBlend()
{
	glEnablei(GL_BLEND, 0);
	glColorMaski(0, GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
	glBlendEquationSeparatei(0, GL_FUNC_REVERSE_SUBTRACT, GL_FUNC_ADD);
	glBlendFuncSeparatei(0, GL_SRC_COLOR, GL_ONE, GL_ZERO, GL_ZERO);

	glEnablei(GL_BLEND, 1);
	glColorMaski(1, GL_TRUE, GL_FALSE, GL_FALSE, GL_FALSE);
	glBlendEquationSeparatei(1, GL_FUNC_ADD, GL_FUNC_ADD);
	glBlendFuncSeparatei(1, GL_SRC_COLOR, GL_SRC_COLOR, GL_ZERO, GL_ZERO);

	glEnablei(GL_BLEND, 2);
	glColorMaski(2, GL_TRUE, GL_FALSE, GL_FALSE, GL_FALSE);
	glBlendEquationSeparatei(2, GL_FUNC_ADD, GL_FUNC_ADD);
	glBlendFuncSeparatei(2, GL_SRC_COLOR, GL_SRC_COLOR, GL_ZERO, GL_ZERO);

	glEnablei(GL_BLEND, 3);
	glColorMaski(3, GL_TRUE, GL_FALSE, GL_FALSE, GL_FALSE);
	glBlendEquationSeparatei(3, GL_FUNC_ADD, GL_FUNC_ADD);
	glBlendFuncSeparatei(3, GL_SRC_COLOR, GL_SRC_COLOR, GL_ZERO, GL_ZERO);

	return glf::checkError("initBlend");
}

bool begin()
{
	Viewport[TEXTURE_RGB8] = glm::ivec4(0, 0, Window.Size >> 1);
	Viewport[TEXTURE_R] = glm::ivec4(Window.Size.x >> 1, 0, Window.Size >> 1);
	Viewport[TEXTURE_G] = glm::ivec4(Window.Size.x >> 1, Window.Size.y >> 1, Window.Size >> 1);
	Viewport[TEXTURE_B] = glm::ivec4(0, Window.Size.y >> 1, Window.Size >> 1);

	GLint MajorVersion = 0;
	GLint MinorVersion = 0;
	glGetIntegerv(GL_MAJOR_VERSION, &MajorVersion);
	glGetIntegerv(GL_MINOR_VERSION, &MinorVersion);
	bool Validated = (MajorVersion * 10 + MinorVersion) >= (SAMPLE_MAJOR_VERSION * 10 + SAMPLE_MINOR_VERSION);

	if(Validated)
		Validated = initBlend();
	if(Validated)
		Validated = initProgram();
	if(Validated)
		Validated = initVertexBuffer();
	if(Validated)
		Validated = initVertexArray();
	if(Validated)
		Validated = initTexture2D();
	if(Validated)
		Validated = initFramebuffer();

	return Validated && glf::checkError("begin");
}

bool end()
{
	glDeleteBuffers(BUFFER_MAX, BufferName);
	glDeleteTextures(TEXTURE_MAX, TextureName);
	glDeleteProgram(ProgramNameMultiple);
	glDeleteProgram(ProgramNameSingle);
	glDeleteFramebuffers(1, &FramebufferName);

	return glf::checkError("end");
}

void display()
{
	// Pass 1
	{
		// Compute the MVP (Model View Projection matrix)
		glm::mat4 Projection = glm::ortho(-1.0f, 1.0f,-1.0f, 1.0f, -1.0f, 1.0f);
		glm::mat4 ViewTranslate = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
		glm::mat4 View = ViewTranslate;
		glm::mat4 Model = glm::mat4(1.0f);
		glm::mat4 MVP = Projection * View * Model;

		glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);
		glViewport(0, 0, FRAMEBUFFER_SIZE.x >> 1, FRAMEBUFFER_SIZE.y >> 1);
		glClearBufferfv(GL_COLOR, 0, &glm::vec4(1.0f)[0]);

		glUseProgram(ProgramNameMultiple);
		glUniformMatrix4fv(UniformMVPMultiple, 1, GL_FALSE, &MVP[0][0]);
		glUniform1i(UniformDiffuseMultiple, 0);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, TextureName[TEXTURE_RGB8]);

		glBindVertexArray(VertexArrayName);
		glDrawElementsBaseVertex(GL_TRIANGLES, ElementCount, GL_UNSIGNED_SHORT, NULL, 0);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	// Pass 2
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearBufferfv(GL_COLOR, 0, &glm::vec4(1.0f)[0]);

	glUseProgram(ProgramNameSingle);

	{
		glm::mat4 Projection = glm::ortho(-1.0f, 1.0f, 1.0f,-1.0f, -1.0f, 1.0f);
		glm::mat4 View = glm::mat4(1.0f);
		glm::mat4 Model = glm::mat4(1.0f);
		glm::mat4 MVP = Projection * View * Model;

		glUniformMatrix4fv(UniformMVPSingle, 1, GL_FALSE, &MVP[0][0]);
		glUniform1i(UniformDiffuseSingle, 0);
	}

	for(std::size_t i = 0; i < TEXTURE_MAX; ++i)
	{
		glViewport(Viewport[i].x, Viewport[i].y, Viewport[i].z, Viewport[i].w);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, TextureName[i]);

		glBindVertexArray(VertexArrayName);
		glDrawElementsBaseVertex(GL_TRIANGLES, ElementCount, GL_UNSIGNED_SHORT, NULL, 0);
	}

	glf::checkError("display");
	glf::swapBuffers();
}

int main(int argc, char* argv[])
{
	if(glf::run(
		argc, argv,
		glm::ivec2(::SAMPLE_SIZE_WIDTH, ::SAMPLE_SIZE_HEIGHT), 
		::SAMPLE_MAJOR_VERSION, 
		::SAMPLE_MINOR_VERSION))
		return 0;
	return 1;
}