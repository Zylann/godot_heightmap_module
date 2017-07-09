filename = "default_shader.txt"

lines = []
with open(filename) as f:
	for line in f:
		lines.append(line)

var_name = filename[:filename.find('.')]
with open("resources.gen.cpp", 'w') as f:
	f.write("\n// This is a generated file. Do not edit.\n")
	f.write("\nconst char *s_" + var_name + "_code =")
	for line in lines:
		# TODO Any better way of escaping those special characters?
		oline = "\n\t\"" + line.replace('\t', "\\t").replace('\n', "\\n") + "\""
		f.write(oline)
	f.write(";\n")

