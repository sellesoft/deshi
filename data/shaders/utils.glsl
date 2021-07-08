#extension GL_EXT_debug_printf : enable

void printMatrix(mat4 matrix){
	debugPrintfEXT("\n%.4f %.4f %.4f %.4f\n%.4f %.4f %.4f %.4f\n%.4f %.4f %.4f %.4f\n%.4f %.4f %.4f %.4f", 
				   matrix[0][0], matrix[0][0], matrix[0][0], matrix[0][0],
				   matrix[0][1], matrix[0][1], matrix[0][1], matrix[0][1],
				   matrix[0][2], matrix[0][2], matrix[0][2], matrix[0][2],
				   matrix[0][3], matrix[0][3], matrix[0][3], matrix[0][3]);
}
