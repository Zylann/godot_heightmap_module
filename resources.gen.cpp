
// This is a generated file. Do not edit.

const char *s_default_shader_code =
	"shader_type spatial;\n"
	"\n"
	"uniform sampler2D height_texture;\n"
	"uniform sampler2D normal_texture;\n"
	"uniform sampler2D color_texture;\n"
	"uniform vec2 heightmap_resolution;\n"
	"uniform mat4 heightmap_inverse_transform;\n"
	"\n"
	"void vertex() {\n"
	"\tvec4 tv = heightmap_inverse_transform * WORLD_MATRIX * vec4(VERTEX, 1);\n"
	"\tvec2 uv = vec2(tv.x,tv.z) / heightmap_resolution;\n"
	"\tfloat h = texture(height_texture, uv).r;\n"
	"\tvec3 n = texture(normal_texture, uv).rgb;\n"
	"\tn = n * 2.0 - vec3(1.0, 1.0, 1.0);\n"
	"\tVERTEX.y = h;\n"
	"\tNORMAL = n;\n"
	"}\n";
