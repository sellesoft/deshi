#include <typeinfo>
#include <cstdio>
#include <ctime>

#define DESHI_ARRAY_GROWTH_FACTOR 2
#define DESHI_ARRAY_SPACE_ALIGNMENT 4
#include "array.h"
function void TEST_deshi_utils_array(){
	//// constructors ////
	array<int> array1;
	AssertAlways(typeid(array1.data) == typeid(int*));
	AssertAlways(array1.count == 0);
	AssertAlways(array1.space == 0);
	AssertAlways(array1.data == 0);
	AssertAlways(array1.first == 0);
	AssertAlways(array1.last == 0);
	AssertAlways(array1.iter == 0);
	
	persist int destruct_sum = 0;
	struct TestType{
		int value;
		TestType(int a){value = a*2;}
		~TestType(){destruct_sum++;}
		bool operator==(const TestType& rhs) const{return value==rhs.value;}
	};
	array<TestType> array2 = array<TestType>();
	AssertAlways(typeid(array2.data) == typeid(TestType*));
	AssertAlways(array2.count == 0);
	AssertAlways(array2.space == 0);
	AssertAlways(array2.data == 0);
	AssertAlways(array2.first == 0);
	AssertAlways(array2.last == 0);
	AssertAlways(array2.iter == 0);
	
	Allocator test_allocator{malloc,Allocator_ChangeMemory_Noop,Allocator_ChangeMemory_Noop,free,realloc};
	array<TestType> array3(&test_allocator);
	AssertAlways(array3.count == 0);
	AssertAlways(array3.space == 0);
	AssertAlways(array3.data == 0);
	AssertAlways(array3.first == 0);
	AssertAlways(array3.last == 0);
	AssertAlways(array3.iter == 0);
	AssertAlways(array3.allocator == &test_allocator);
	AssertAlways(array3.allocator->reserve == malloc);
	
	array<TestType> array4(5);
	AssertAlways(array4.count == 0);
	AssertAlways(array4.space == 8);
	AssertAlways(array4.data != 0);
	AssertAlways(array4.first == 0);
	AssertAlways(array4.last == 0);
	AssertAlways(array4.iter == 0);
	AssertAlways(array4.data[0].value == 0 && array4.data[1].value == 0 && array4.data[2].value == 0 && array4.data[3].value == 0);
	AssertAlways(array4.data[4].value == 0 && array4.data[5].value == 0 && array4.data[6].value == 0 && array4.data[7].value == 0);
	
	array<TestType> array5({TestType(1), TestType(2), TestType(3)});
	AssertAlways(destruct_sum == 3);
	AssertAlways(array5.count == 3);
	AssertAlways(array5.space == 4);
	AssertAlways(array5.data != 0);
	AssertAlways(array5.first == array5.data);
	AssertAlways(array5.last == array5.data+2);
	AssertAlways(array5.iter == array5.data);
	AssertAlways(array5.data[0].value == 2);
	AssertAlways(array5.data[1].value == 4);
	AssertAlways(array5.data[2].value == 6);
	AssertAlways(PointerDifference(&array5.data[1], &array5.data[0]) == sizeof(TestType));
	AssertAlways(array5.data[3].value == 0);
	
	array<TestType> array6(array5);
	AssertAlways(array6.count == 3);
	AssertAlways(array6.space == 4);
	AssertAlways(array6.data != 0);
	AssertAlways(array6.first == array6.data);
	AssertAlways(array6.last == array6.data+2);
	AssertAlways(array6.iter == array6.data);
	AssertAlways(array6.data[0].value == 2);
	AssertAlways(array6.data[1].value == 4);
	AssertAlways(array6.data[2].value == 6);
	
	array<TestType> array7(array5.data, 2);
	AssertAlways(array7.count == 2);
	AssertAlways(array7.space == 4);
	AssertAlways(array7.data != 0);
	AssertAlways(array7.first == array7.data);
	AssertAlways(array7.last == array7.data+1);
	AssertAlways(array7.iter == array7.data);
	AssertAlways(array7.data[0].value == 2);
	AssertAlways(array7.data[1].value == 4);
	AssertAlways(array7.data[2].value == 0);
	
	array7.~array();
	AssertAlways(destruct_sum == 5);
	array7.data = 0;
	
	//// operators ////
	array3 = array5;
	AssertAlways(destruct_sum == 5);
	AssertAlways(array3.count == 3);
	AssertAlways(array3.space == 4);
	AssertAlways(array3.data != 0 && array3.data != array5.data);
	AssertAlways(array3.first == array3.data);
	AssertAlways(array3.last == array3.data+2);
	AssertAlways(array3.iter == array3.data);
	AssertAlways(array3.allocator == DESHI_ARRAY_ALLOCATOR);
	AssertAlways(array3.data[0].value == 2);
	AssertAlways(array3.data[1].value == 4);
	AssertAlways(array3.data[2].value == 6);
	
	array3[0] = TestType(5);
	AssertAlways(destruct_sum == 6);
	AssertAlways(array3.data[0].value == 10);
	AssertAlways(array3.data[1].value == 4);
	AssertAlways(array3.data[2].value == 6);
	
	AssertAlways(array3[0].value == 10);
	AssertAlways(array3[1].value == 4);
	AssertAlways(array3[2].value == 6);
	
	//// functions ////
	array2.add(TestType(1));
	AssertAlways(destruct_sum == 7);
	AssertAlways(array2.count == 1);
	AssertAlways(array2.space == 4);
	AssertAlways(array2.data != 0);
	AssertAlways(array2.first == array2.data);
	AssertAlways(array2.last == array2.data);
	AssertAlways(array2.iter == array2.data);
	AssertAlways(array2[0].value == 2);
	AssertAlways(array2.data[1].value == 0);
	
	array2.add(TestType(2));
	array2.add(TestType(3));
	array2.add(TestType(4));
	AssertAlways(destruct_sum == 10);
	AssertAlways(array2.count == 4);
	AssertAlways(array2.space == 4);
	AssertAlways(array2.data != 0);
	AssertAlways(array2.last == array2.data+3);
	AssertAlways(array2[0].value == 2);
	AssertAlways(array2[1].value == 4);
	AssertAlways(array2[2].value == 6);
	AssertAlways(array2[3].value == 8);
	
	array2.iter++;
	array2.add(TestType(5));
	AssertAlways(destruct_sum == 11);
	AssertAlways(array2.count == 5);
	AssertAlways(array2.space == 8);
	AssertAlways(array2.data != 0);
	AssertAlways(array2.first == array2.data);
	AssertAlways(array2.last == array2.data+4);
	AssertAlways(array2.iter == array2.data+1);
	AssertAlways(array2[0].value == 2);
	AssertAlways(array2[1].value == 4);
	AssertAlways(array2[2].value == 6);
	AssertAlways(array2[3].value == 8);
	AssertAlways(array2[4].value == 10);
	
	array2.add_array(array6);
	AssertAlways(array2.count == 8);
	AssertAlways(array2.space == 8);
	AssertAlways(array2.last == array2.data+7);
	AssertAlways(array2[0].value == 2);
	AssertAlways(array2[1].value == 4);
	AssertAlways(array2[2].value == 6);
	AssertAlways(array2[3].value == 8);
	AssertAlways(array2[4].value == 10);
	AssertAlways(array2[5].value == 2);
	AssertAlways(array2[6].value == 4);
	AssertAlways(array2[7].value == 6);
	
	array<TestType> array8;
	array8.emplace(10);
	AssertAlways(destruct_sum == 12);
	AssertAlways(array8.count == 1);
	AssertAlways(array8.space == 4);
	AssertAlways(array8.first == array8.data);
	AssertAlways(array8.last == array8.data);
	AssertAlways(array8.iter == array8.data);
	AssertAlways(array8[0].value == 20);
	AssertAlways(array8.data[1].value == 0);
	
	array2.emplace(1);
	AssertAlways(destruct_sum == 13);
	AssertAlways(array2.count == 9);
	AssertAlways(array2.space == 16);
	AssertAlways(array2.first == array2.data);
	AssertAlways(array2.last == array2.data+8);
	AssertAlways(array2.iter == array2.data+1);
	AssertAlways(array2[0].value == 2);
	AssertAlways(array2[1].value == 4);
	AssertAlways(array2[8].value == 2);
	AssertAlways(array2.data[9].value == 0);
	
	array2.emplace(5);
	AssertAlways(destruct_sum == 14);
	AssertAlways(array2.count == 10);
	AssertAlways(array2.space == 16);
	AssertAlways(array2.last == array2.data+9);
	AssertAlways(array2[0].value == 2);
	AssertAlways(array2[1].value == 4);
	AssertAlways(array2[8].value == 2);
	AssertAlways(array2[9].value == 10);
	AssertAlways(array2.data[10].value == 0);
	
	array<TestType> array9;
	array9.insert(TestType(1), 0);
	AssertAlways(destruct_sum == 15);
	AssertAlways(array9.count == 1);
	AssertAlways(array9.space == 4);
	AssertAlways(array9.first == array9.data);
	AssertAlways(array9.last == array9.data);
	AssertAlways(array9.iter == array9.data);
	AssertAlways(array9[0].value == 2);
	AssertAlways(array9.data[1].value == 0);
	
	array9.insert(TestType(3), 1);
	AssertAlways(destruct_sum == 16);
	AssertAlways(array9.count == 2);
	AssertAlways(array9.space == 4);
	AssertAlways(array9.last == array9.data+1);
	AssertAlways(array9[0].value == 2);
	AssertAlways(array9[1].value == 6);
	AssertAlways(array9.data[2].value == 0);
	
	array9.insert(TestType(2), 1);
	AssertAlways(destruct_sum == 17);
	AssertAlways(array9.count == 3);
	AssertAlways(array9.space == 4);
	AssertAlways(array9.last == array9.data+2);
	AssertAlways(array9[0].value == 2);
	AssertAlways(array9[1].value == 4);
	AssertAlways(array9[2].value == 6);
	AssertAlways(array9.data[3].value == 0);
	
	array9.emplace(4);
	array9.insert(TestType(5), 4);
	AssertAlways(destruct_sum == 19);
	AssertAlways(array9.count == 5);
	AssertAlways(array9.space == 8);
	AssertAlways(array9.first == array9.data);
	AssertAlways(array9.last == array9.data+4);
	AssertAlways(array9.iter == array9.data);
	AssertAlways(array9[0].value == 2);
	AssertAlways(array9[1].value == 4);
	AssertAlways(array9[2].value == 6);
	AssertAlways(array9[3].value == 8);
	AssertAlways(array9[4].value == 10);
	AssertAlways(array9.data[5].value == 0);
	
	array9.pop();
	AssertAlways(destruct_sum == 20);
	AssertAlways(array9.count == 4);
	AssertAlways(array9.space == 8);
	AssertAlways(array9.last == array9.data+3);
	AssertAlways(array9[0].value == 2);
	AssertAlways(array9[1].value == 4);
	AssertAlways(array9[2].value == 6);
	AssertAlways(array9[3].value == 8);
	AssertAlways(array9.data[4].value == 0);
	AssertAlways(array9.data[5].value == 0);
	
	array9.pop(2);
	AssertAlways(destruct_sum == 22);
	AssertAlways(array9.count == 2);
	AssertAlways(array9.space == 8);
	AssertAlways(array9.last == array9.data+1);
	AssertAlways(array9[0].value == 2);
	AssertAlways(array9[1].value == 4);
	AssertAlways(array9.data[2].value == 0);
	AssertAlways(array9.data[3].value == 0);
	AssertAlways(array9.data[4].value == 0);
	AssertAlways(array9.data[5].value == 0);
	
	array9.pop(2);
	AssertAlways(destruct_sum == 24);
	AssertAlways(array9.count == 0);
	AssertAlways(array9.space == 8);
	AssertAlways(array9.first == 0);
	AssertAlways(array9.last == 0);
	AssertAlways(array9.iter == 0);
	AssertAlways(array9.data[0].value == 0);
	AssertAlways(array9.data[1].value == 0);
	
	array9.emplace(1);
	array9.emplace(2);
	array9.remove(0);
	AssertAlways(destruct_sum == 27);
	AssertAlways(array9.count == 1);
	AssertAlways(array9.space == 8);
	AssertAlways(array9.last == array9.data);
	AssertAlways(array9[0].value == 4);
	AssertAlways(array9.data[1].value == 0);
	
	array9.remove(0);
	AssertAlways(destruct_sum == 28);
	AssertAlways(array9.count == 0);
	AssertAlways(array9.space == 8);
	AssertAlways(array9.first == 0);
	AssertAlways(array9.last == 0);
	AssertAlways(array9.iter == 0);
	AssertAlways(array9.data[0].value == 0);
	
	array2.clear();
	AssertAlways(destruct_sum == 38);
	AssertAlways(array2.count == 0);
	AssertAlways(array2.space == 16);
	AssertAlways(array2.first == 0);
	AssertAlways(array2.last == 0);
	AssertAlways(array2.iter == 0);
	AssertAlways(array2.data[0].value == 0);
	AssertAlways(array2.data[10].value == 0);
	
	array5.iter++;
	array5.resize(6);
	AssertAlways(array5.count == 6);
	AssertAlways(array5.space == 6);
	AssertAlways(array5.data != 0);
	AssertAlways(array5.first == array5.data);
	AssertAlways(array5.last == array5.data+5);
	AssertAlways(array5.iter == array5.data+1);
	AssertAlways(array5[0].value == 2);
	AssertAlways(array5[1].value == 4);
	AssertAlways(array5[2].value == 6);
	AssertAlways(array5.data[3].value == 0);
	AssertAlways(array5.data[5].value == 0);
	
	array5.resize(2);
	AssertAlways(array5.count == 2);
	AssertAlways(array5.space == 2);
	AssertAlways(array5.data != 0);
	AssertAlways(array5.first == array5.data);
	AssertAlways(array5.last == array5.data+1);
	AssertAlways(array5.iter == array5.data+1);
	AssertAlways(array5[0].value == 2);
	AssertAlways(array5[1].value == 4);
	
	array5.reserve(3);
	AssertAlways(array5.count == 2);
	AssertAlways(array5.space == 4);
	AssertAlways(array5.data != 0);
	AssertAlways(array5.first == array5.data);
	AssertAlways(array5.last == array5.data+1);
	AssertAlways(array5.iter == array5.data+1);
	AssertAlways(array5[0].value == 2);
	AssertAlways(array5[1].value == 4);
	
	array5.reserve(3);
	AssertAlways(array5.count == 2);
	AssertAlways(array5.space == 4);
	AssertAlways(array5.data != 0);
	AssertAlways(array5.first == array5.data);
	AssertAlways(array5.last == array5.data+1);
	AssertAlways(array5.iter == array5.data+1);
	AssertAlways(array5[0].value == 2);
	AssertAlways(array5[1].value == 4);
	
	array5.swap(0,1);
	AssertAlways(array5[0].value == 4);
	AssertAlways(array5[1].value == 2);
	
	array5.swap(0,1);
	AssertAlways(array5[0].value == 2);
	AssertAlways(array5[1].value == 4);
	
	AssertAlways(array5.has(TestType(2)));
	AssertAlways(!array5.has(TestType(4)));
	
	array5.at(0) = TestType(5);
	AssertAlways(array3[0].value == 10);
	AssertAlways(array3[1].value == 4);
	
	AssertAlways(array3.at(0).value == 10);
	AssertAlways(array3.at(1).value == 4);
	
	//TODO(sushi) setup array special pointer testing
	
	printf("[DESHI-TEST] PASSED: utils/array\n");
}

#include "array_algorithms.h"
function void TEST_deshi_utils_array_algorithms(){
	srand(time(0));
	
	//bubble sort
	array<s32> array1(1024);
	forI(1024) array1.add(rand() % 1024);
	bubble_sort(array1, [](s32 a, s32 b){return a < b;});
	forI(1024){ if(i){ AssertAlways(array1[i] <= array1[i-1]); } }
	
	srand(time(0));
	array1.clear();
	forI(1024) array1.add(rand() % 1024);
	bubble_sort_low_to_high(array1);
	forI(1024){ if(i){ AssertAlways(array1[i] >= array1[i-1]); } }
	
	srand(time(0));
	array1.clear();
	forI(1024) array1.add(rand() % 1024);
	bubble_sort_high_to_low(array1);
	forI(1024){ if(i){ AssertAlways(array1[i] <= array1[i-1]); } }
	
	//reverse
	reverse(array1);
	forI(1024){ if(i){ AssertAlways(array1[i] >= array1[i-1]); } }
	
	//binary search
	array1[0] = MAX_S32;
	array1[84] = MIN_S32;
	array1[123] = 0;
	bubble_sort_low_to_high(array1);
	AssertAlways(binary_search(array1, MAX_S32) != -1);
	AssertAlways(binary_search(array1, MIN_S32) != -1);
	AssertAlways(binary_search(array1, 0) != -1);
	
	printf("[DESHI-TEST] PASSED: utils/array_algorithms\n");
}

#include "carray.h"
function void TEST_deshi_utils_carray(){
	int* arr0 = (int*)calloc(1, 16*sizeof(int));
	defer{ free(arr0); };
	forI(16){ arr0[i] = 1 << i; }
	
	carray<int> arr1{arr0, 16};
	AssertAlways(arr1.data == arr0);
	AssertAlways(arr1.count == 16);
	AssertAlways(arr1);
	AssertAlways(arr1[0] == 1 << 0);
	AssertAlways(arr1[1] == 1 << 1);
	AssertAlways(arr1[4] == 1 << 4);
	AssertAlways(arr1[15] == 1 << 15);
	AssertAlways(arr1.at(8) == arr0+8);
	forE(arr1){ AssertAlways(*it == 1 << (it - it_begin)); }
	
	array_pop(arr1, 2);
	AssertAlways(arr1.count == 16);
	arr1.count = 14;
	AssertAlways(arr1);
	AssertAlways(arr1[0] == 1 << 0);
	AssertAlways(arr1[1] == 1 << 1);
	AssertAlways(arr1[4] == 1 << 4);
	AssertAlways(arr1[13] == 1 << 13);
	forE(arr1){ AssertAlways(*it == 1 << (it - it_begin)); }
	
	array_remove_unordered(arr1, 4);
	AssertAlways(arr1.data == arr0);
	AssertAlways(arr1.count == 14);
	arr1.count = 13;
	AssertAlways(arr1);
	AssertAlways(arr1[0] == 1 << 0);
	AssertAlways(arr1[1] == 1 << 1);
	AssertAlways(arr1[4] == 1 << 13);
	AssertAlways(arr1[12] == 1 << 12);
	
	array_remove_ordered(arr1, 4);
	AssertAlways(arr1.data == arr0);
	AssertAlways(arr1.count == 13);
	arr1.count = 12;
	AssertAlways(arr1);
	AssertAlways(arr1[0] == 1 << 0);
	AssertAlways(arr1[1] == 1 << 1);
	AssertAlways(arr1[4] == 1 << 5);
	AssertAlways(arr1[11] == 1 << 12);
	
	printf("[DESHI-TEST] PASSED: utils/carray\n");
}

#include "color.h"
function void TEST_deshi_utils_color(){
	//// constructors ////
	color color1;
	AssertAlways(color1.r == 000 && color1.g == 000 && color1.b == 000 && color1.a == 000);
	AssertAlways(color1.rgba == 0);
	
	color color2(0, 255, 0, 255);
	AssertAlways(color2.r == 000 && color2.g == 255 && color2.b == 000 && color2.a == 255);
	AssertAlways(color2.rgba == 0xFF00FF00);
	
	color color3(0xFFFF0000);
	AssertAlways(color3.r == 255 && color3.g == 255 && color3.b == 000 && color3.a == 000);
	AssertAlways(color3.rgba == 0x0000FFFF);
	
	color color4 = Color_Red;
	AssertAlways(color4.r == 255 && color4.g == 000 && color4.b == 000 && color4.a == 255);
	AssertAlways(color4.rgba == 0xFF0000FF);
	
	//// macros ////
	AssertAlways((Color_Blue.rgba & COLORU32_RMASK) == 0);
	AssertAlways((Color_Blue.rgba & COLORU32_GMASK) == 0);
	AssertAlways((Color_Blue.rgba & COLORU32_BMASK) == COLORU32_BMASK);
	AssertAlways((Color_Blue.rgba & COLORU32_AMASK) == COLORU32_AMASK);
	
	AssertAlways((Color_Green.rgba & COLORU32_RMASK) == 0);
	AssertAlways((Color_Green.rgba & COLORU32_GMASK) == COLORU32_GMASK);
	AssertAlways((Color_Green.rgba & COLORU32_BMASK) == 0);
	AssertAlways((Color_Green.rgba & COLORU32_AMASK) == COLORU32_AMASK);
	
	AssertAlways(Color_Green == PackColorU32(0,255,0,255));
	AssertAlways(color(0,255,0,255) == PackColorU32(0,255,0,255));
	
	//// operators ////
	color color5(32, 32, 32, 128);
	color5 *= 2;
	AssertAlways(color5.r == 64 && color5.g == 64 && color5.b == 64 && color5.a == 128);
	
	AssertAlways(color(255, 0, 0, 255) == Color_Red);
	AssertAlways(Color_Blue == 0x0000FFFF);
	
	AssertAlways(color5*color(2,0,0,2) == color(128,0,0,128));
	
	AssertAlways(color5*2 == color(128,128,128,128));
	
	AssertAlways(color5/2 == color(32,32,32,128));
	
	//// functions ////
	color color6 = color::FloatsToColor(0,1,0,1);
	AssertAlways(color6 == Color_Green);
	color6 = color::FloatsToColor(0,.5f,0,1);
	AssertAlways(color6 == Color_Green/2);
	
	f32 test_arr1[4] = {1.f, 1.f, 1.f, 1.f};
	color::FillFloat3FromU32(test_arr1, Color_Blue.rgba);
	AssertAlways(test_arr1[0] == 0.f);
	AssertAlways(test_arr1[1] == 0.f);
	AssertAlways(test_arr1[2] == 1.f);
	AssertAlways(test_arr1[3] == 1.f);
	
	f32 test_arr2[4] = {0, 0, 1.f, 0};
	color::FillFloat4FromU32(test_arr2, color(128,128,0,128).rgba);
	AssertAlways(abs(test_arr2[0] - .5f) <= .01f);
	AssertAlways(abs(test_arr2[1] - .5f) <= .01f);
	AssertAlways(test_arr2[2] == 0.f);
	AssertAlways(abs(test_arr2[3] - .5f) <= .01f);
	
	printf("[DESHI-TEST] PASSED: utils/color\n");
}

#include "cstring.h"
function void TEST_deshi_utils_cstring(){
	printf("[DESHI-TEST] TODO:   utils/cstring\n");
}

#include "hash.h"
function void TEST_deshi_utils_hash(){
	printf("[DESHI-TEST] TODO:   utils/hash\n");
}

#include "map.h"
function void TEST_deshi_utils_map(){
	printf("[DESHI-TEST] TODO:   utils/map\n");
}

#include "optional.h"
function void TEST_deshi_utils_optional(){
	printf("[DESHI-TEST] TODO:   utils/optional\n");
}

#include "ring_array.h"
function void TEST_deshi_utils_ring_array(){
	printf("[DESHI-TEST] TODO:   utils/ring_array\n");
}

#include "string.h"
function void TEST_deshi_utils_string(){
	
	{//empty constructor does not allocate or set any vars
		string str;
		AssertAlways(!str.str);
		AssertAlways(!str.count);
		AssertAlways(!str.space);
	}

	{//const char* constructor
		string str("ABCDEFG");
		AssertAlways(!memcmp(str.str, "ABCDEFG", 7));
		AssertAlways(str.count == 7);
		AssertAlways(str.space == 8);

	}


}

#include "string_conversion.h"
function void TEST_deshi_utils_string_conversion(){
	printf("[DESHI-TEST] TODO:   utils/string_conversion\n");
}

#include "tuple.h"
function void TEST_deshi_utils_tuple(){
	printf("[DESHI-TEST] TODO:   utils/tuple\n");
}

#include "utils.h"
function void TEST_deshi_utils_utils(){
	printf("[DESHI-TEST] TODO:   utils/utils\n");
}

function void TEST_deshi_utils(){
	TEST_deshi_utils_array();
	TEST_deshi_utils_array_algorithms();
	TEST_deshi_utils_carray();
	TEST_deshi_utils_color();
	TEST_deshi_utils_cstring();
	TEST_deshi_utils_hash();
	TEST_deshi_utils_map();
	TEST_deshi_utils_optional();
	TEST_deshi_utils_ring_array();
	TEST_deshi_utils_string();
	TEST_deshi_utils_string_conversion();
	TEST_deshi_utils_tuple();
	TEST_deshi_utils_utils();

	
}