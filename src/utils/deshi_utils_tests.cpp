#include <typeinfo>
#include <cstdio>
#include <ctime>

#define DESHI_ARRAY_GROWTH_FACTOR 2
#define DESHI_ARRAY_SPACE_ALIGNMENT 4
#include "array.h"
local void TEST_deshi_utils_array(){
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
	AssertAlways(array2.first == array2.data);
	AssertAlways(array2.last == 0);
	AssertAlways(array2.iter == array2.data);
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

#include "array_utils.h"
local void TEST_deshi_utils_array_utils(){
#define PRINT_ARRAY_SPEEDS true
	TIMER_START(timer);
	
	//bubble sort
	srand(time(0));
	array<s32> array1(1024);
	forI(1024) array1.add(rand() % 1024);
	TIMER_RESET(timer);
	bubble_sort(array1, [](s32 a, s32 b){return a < b;});
#if PRINT_ARRAY_SPEEDS
	Log("deshi-test","bubble_sort() took ",TIMER_END(timer),"ms");
#endif
	forI(1024){ if(i){ AssertAlways(array1[i] <= array1[i-1]); } }
	printf("[DESHI-TEST] PASSED: utils/array_utils/bubble_sort()\n");
	
	srand(time(0));
	array1.clear();
	forI(1024) array1.add(rand() % 1024);
	TIMER_RESET(timer);
	bubble_sort_low_to_high(array1);
#if PRINT_ARRAY_SPEEDS
	Log("deshi-test","bubble_sort_low_to_high() took ",TIMER_END(timer),"ms");
#endif
	forI(1024){ if(i){ AssertAlways(array1[i] >= array1[i-1]); } }
	printf("[DESHI-TEST] PASSED: utils/array_utils/bubble_sort_low_to_high()\n");
	
	srand(time(0));
	array1.clear();
	forI(1024) array1.add(rand() % 1024);
	TIMER_RESET(timer);
	bubble_sort_high_to_low(array1);
#if PRINT_ARRAY_SPEEDS
	Log("deshi-test","bubble_sort_high_to_low() took ",TIMER_END(timer),"ms");
#endif
	forI(1024){ if(i){ AssertAlways(array1[i] <= array1[i-1]); } }
	printf("[DESHI-TEST] PASSED: utils/array_utils/bubble_sort_high_to_low()\n");
	
	//reverse
	TIMER_RESET(timer);
	reverse(array1);
#if PRINT_ARRAY_SPEEDS
	Log("deshi-test","reverse() took ",TIMER_END(timer),"ms");
#endif
	forI(1024){ if(i){ AssertAlways(array1[i] >= array1[i-1]); } }
	printf("[DESHI-TEST] PASSED: utils/array_utils/reverse()\n");
	
	//binary search
	//TODO test binary search comparator
	
	array1[0] = MAX_S32;
	array1[84] = MIN_S32;
	array1[123] = 0;
	bubble_sort_low_to_high(array1);
	AssertAlways(binary_search_low_to_high(array1, MAX_S32) != -1);
	AssertAlways(binary_search_low_to_high(array1, MIN_S32) != -1);
	AssertAlways(binary_search_low_to_high(array1, 0) != -1);
	printf("[DESHI-TEST] PASSED: utils/array_utils/binary_search_low_to_high()\n");
	
	printf("[DESHI-TEST] PASSED: utils/array_utils\n");
}

#include "carray.h"
local void TEST_deshi_utils_carray(){
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
local void TEST_deshi_utils_color(){
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
local void TEST_deshi_utils_cstring(){
	printf("[DESHI-TEST] TODO:   utils/cstring\n");
}

#include "hash.h"
local void TEST_deshi_utils_hash(){
	printf("[DESHI-TEST] TODO:   utils/hash\n");
}

#include "map.h"
local void TEST_deshi_utils_map(){
	printf("[DESHI-TEST] TODO:   utils/map\n");
}

#include "optional.h"
local void TEST_deshi_utils_optional(){
	printf("[DESHI-TEST] TODO:   utils/optional\n");
}

#include "ring_array.h"
local void TEST_deshi_utils_ring_array(){
	printf("[DESHI-TEST] TODO:   utils/ring_array\n");
}

#include "string.h"
local void TEST_deshi_utils_string(){
	
	
	//#define teststr "ABCDEFG"
#define teststr "While the butterflies form a monophyletic group, the moths, comprising the rest of the Lepidoptera, do not. Many attempts have been made to group the superfamilies of the Lepidoptera into natural groups, most of which fail because one of the two groups is not monophyletic: Microlepidoptera and Macrolepidoptera, Heterocera and Rhopalocera, Jugatae and Frenatae, Monotrysia and Ditrysia.\nAlthough the rules for distinguishing moths from butterflies are not well established, one very good guiding principle is that butterflies have thin antennae and (with the exception of the family Hedylidae) have small balls or clubs at the end of their antennae.Moth antennae are usually feathery with no ball on the end.The divisions are named by this principle: \"club-antennae\" (Rhopalocera) or \"varied-antennae\" (Heterocera).Lepidoptera differs between butterflies and other organisms due to evolving a special characteristic of having the tube - like proboscis in the Middle Triassic which allowed them to acquire nectar from flowering plants.[3] "
	const u32 len = strlen(teststr);
	
	//// constructors ////
	{//empty constructor does not allocate or set any vars
		string str;
		AssertAlways(!str.str);
		AssertAlways(!str.count);
		AssertAlways(!str.space);
	}
	
	{//const char* constructor
		string str(teststr);
		AssertAlways(!memcmp(str.str, teststr, len));
		AssertAlways(str.count == len);
		AssertAlways(str.space == RoundUpTo(len + 1, 4));
	}
	
	{//const char* with count constructor
		string str(teststr, len);
		AssertAlways(!memcmp(str.str, teststr, len));
		AssertAlways(str.count == len);
		AssertAlways(str.space == RoundUpTo(len + 1, 4));
	}
	
	{//copy constructor
		string str(teststr);
		string str2(str);
		AssertAlways(!memcmp(str2.str, teststr, len));
		AssertAlways(str2.count == len);
		AssertAlways(str2.space == RoundUpTo(len + 1, 4));
	}
	
	
	//// operators ////
	
	
	{//operator[](u32)
		string str(teststr);
		for (u32 i = 0; i < len; i++)
			AssertAlways(str[i] == teststr[i]);
	}
	
	{//operator[] const
		const string str(teststr);
		for (u32 i = 0; i < len; i++)
			AssertAlways(str[i] == teststr[i]);
	}
	
	{//operator= const char*
		string str = teststr;
		AssertAlways(!memcmp(str.str, teststr, len));
		AssertAlways(str.count == len);
		AssertAlways(str.space == RoundUpTo(len + 1, 4));
	}
	
	{//operator= string
		string str = teststr;
		string str2 = str;
		AssertAlways(!memcmp(str2.str, teststr, len));
		AssertAlways(str2.count == len);
		AssertAlways(str2.space == RoundUpTo(len + 1, 4));
	}
	
	{//operator+= const char*
		string str = teststr;
		str += teststr;
		AssertAlways(!memcmp(str.str, teststr teststr, len * 2));
		AssertAlways(str.count == len * 2);
		AssertAlways(str.space == RoundUpTo(len * 2 + 1, 4));
	}
	
	{//operator+= string
		string str = teststr;
		string str2 = teststr;
		str += str2;
		AssertAlways(!memcmp(str.str, teststr teststr, len * 2));
		AssertAlways(str.count == len * 2);
		AssertAlways(str.space == RoundUpTo(len * 2 + 1 , 4));
	}
	
	{//operator--
		string str = teststr;
		str--;
		for (u32 i = 0; i < len - 1; i++)
			AssertAlways(str[i] == teststr[i]);
		AssertAlways(str.count == len - 1);
		AssertAlways(str.space == RoundUpTo(len + 1, 4));
	}
	
	{//operator+ const char*
		string str = teststr;
		string str2 = str + teststr;
		AssertAlways(!memcmp(str2.str, teststr teststr, len * 2));
		AssertAlways(str2.count == len * 2);
		AssertAlways(str2.space == RoundUpTo(len * 2 + 1, 4));
	}
	
	{//operator+ string
		string str = teststr;
		string str2 = teststr;
		string str3 = str + str2;
		AssertAlways(!memcmp(str3.str, teststr teststr, len * 2));
		AssertAlways(str3.count == len * 2);
		AssertAlways(str3.space == RoundUpTo(len * 2 + 1, 4));
	}
	
	{//operator== string
		string str = teststr;
		string str2 = teststr;
		AssertAlways(str == str2);
	}
	
	{//operator!= string
		string str = teststr;
		string str2 = teststr "a";
		AssertAlways(str != str2);
	}
	
	{//operator== const char*
		string str = teststr;
		AssertAlways(str == teststr);
	}
	
	{//operator!= const char*
		string str = teststr;
		AssertAlways(str != teststr"a");
	}
	
	{//friend operator+ const char* and string
		string str = "yep";
		string str2 = teststr + str;
		AssertAlways(!memcmp(str2.str, teststr "yep", len + 3));
		AssertAlways(str2.count == len+3);
		AssertAlways(str2.space == RoundUpTo(len + 3 + 1, 4));
	}
	
	{//operator bool()
		string str;
		AssertAlways(!str);
		str = teststr;
		AssertAlways(str);
	}
	
	
	//// functions ////
	
	
	{//reserve
		string str;
		str.reserve(len);
		AssertAlways(str.str);
		AssertAlways(!str.count);
		AssertAlways(str.space == RoundUpTo(len + 1, 4));
		
		str.reserve(len - 1);
		AssertAlways(str.str);
		AssertAlways(!str.count);
		AssertAlways(str.space == RoundUpTo(len + 1, 4));
		
		//forI(len) str[i] = teststr[i];
		//AssertAlways(!memcmp(str.str, teststr, len));
		//AssertAlways(str.count == len);
		//AssertAlways(str.space == RoundUpTo(len + 1, 4));
	}
	
	{//clear
		string str = teststr;
		str.clear();
		AssertAlways(!str.str);
		AssertAlways(!str.count);
		AssertAlways(!str.space);
	}
	
	{//erase
		string str;
		for (u32 i = 0; i < len; i++) {
			str = teststr;
			str.erase(i);
			for (u32 o = 0; o < len; o++) {
				if (o != i)
					AssertAlways(str[(o >= i ? o - 1 : o)] == teststr[o]);
			}
		}
		
		str = teststr;
		while (str) str.erase(0);
		AssertAlways(!str.str);
		AssertAlways(!str.count);
		AssertAlways(!str.space);
		
		str = teststr;
		srand(time(0));
		while (str) str.erase(rand() % str.count); 
		AssertAlways(!str.str);
		AssertAlways(!str.count);
		AssertAlways(!str.space);
	}
	
	{//insert
		string str;
		for (u32 i = 0; i < len; i++) {
			str = teststr;
			str.insert('A', i);
			for (u32 o = 0; o < len+1; o++) {
				if (o != i)
					AssertAlways(str[o] == teststr[(o >= i ? o - 1 : o)]);
			}
			
		}
		
	}
	
	{//at
		string str = teststr;
		forI(str.count) AssertAlways(str.at(i) == str[i]);
	}
	
	{//substr
		string str = teststr;
		
		forI(1000) {
			s32 rand1 = rand() % str.count,
			rand2 = rand() % str.count;
			string str2 = str.substr(Min(rand1, rand2), Max(rand1, rand2));
			AssertAlways(!memcmp(str.str + Min(rand1, rand2), str2.str, (upt)fabs(rand1 - rand2)));
		}
		
		
	}
	
	{//findFirstStr
		string str = teststr;
		
		
	}
	
	//TODO(sushi) write tests for the remaining functions
	//TODO write tests for wstring
	
	printf("[DESHI-TEST] PASSED: utils/string\n");
}

#include "string_utils.h"
local void TEST_deshi_utils_string_utils(){
	printf("[DESHI-TEST] TODO:   utils/string_utils\n");
}

#include "pair.h"
local void TEST_deshi_utils_pair(){
	printf("[DESHI-TEST] TODO:   utils/pair\n");
}

#include "unicode.h"
local void TEST_deshi_utils_unicode(){
#define UNICODE_BASIC_LATIN "! \" # $ % & ' ( ) * + , - . / 0 1 2 3 4 5 6 7 8 9 : ; < = > ? @ A B C D E F G H I J K L M N O P Q R S T U V W X Y Z [ \ ] ^ _ ` a b c d e f g h i j k l m n o p q r s t u v w x y z { | } ~"
#define UNICODE_LATIN_SUPPLEMENT "¡ ¢ £ ¤ ¥ ¦ § ¨ © ª « ¬ ­ ® ¯ ° ± ² ³ ´ µ ¶ · ¸ ¹ º » ¼ ½ ¾ ¿ À Á Â Ã Ä Å Æ Ç È É Ê Ë Ì Í Î Ï Ð Ñ Ò Ó Ô Õ Ö × Ø Ù Ú Û Ü Ý Þ ß à á â ã ä å æ ç è é ê ë ì í î ï ð ñ ò ó ô õ ö ÷ ø ù ú û ü ý þ ÿ"
#define UNICODE_LATIN_EXTENDED_A "Ā ā Ă ă Ą ą Ć ć Ĉ ĉ Ċ ċ Č č Ď ď Đ đ Ē ē Ĕ ĕ Ė ė Ę ę Ě ě Ĝ ĝ Ğ ğ Ġ ġ Ģ ģ Ĥ ĥ Ħ ħ Ĩ ĩ Ī ī Ĭ ĭ Į į İ ı Ĳ ĳ Ĵ ĵ Ķ ķ ĸ Ĺ ĺ Ļ ļ Ľ ľ Ŀ ŀ Ł ł Ń ń Ņ ņ Ň ň ŉ Ŋ ŋ Ō ō Ŏ ŏ Ő ő Œ œ Ŕ ŕ Ŗ ŗ Ř ř Ś ś Ŝ ŝ Ş ş Š š Ţ ţ Ť ť Ŧ ŧ Ũ ũ Ū ū Ŭ ŭ Ů ů Ű ű Ų ų Ŵ ŵ Ŷ ŷ Ÿ Ź ź Ż ż Ž ž ſ"
#define UNICODE_LATIN_EXTENDED_B "ƀ Ɓ Ƃ ƃ Ƅ ƅ Ɔ Ƈ ƈ Ɖ Ɗ Ƌ ƌ ƍ Ǝ Ə Ɛ Ƒ ƒ Ɠ Ɣ ƕ Ɩ Ɨ Ƙ ƙ ƚ ƛ Ɯ Ɲ ƞ Ɵ Ơ ơ Ƣ ƣ Ƥ ƥ Ʀ Ƨ ƨ Ʃ ƪ ƫ Ƭ ƭ Ʈ Ư ư Ʊ Ʋ Ƴ ƴ Ƶ ƶ Ʒ Ƹ ƹ ƺ ƻ Ƽ ƽ ƾ ƿ ǀ ǁ ǂ ǃ Ǆ ǅ ǆ Ǉ ǈ ǉ Ǌ ǋ ǌ Ǎ ǎ Ǐ ǐ Ǒ ǒ Ǔ ǔ Ǖ ǖ Ǘ ǘ Ǚ ǚ Ǜ ǜ ǝ Ǟ ǟ Ǡ ǡ Ǣ ǣ Ǥ ǥ Ǧ ǧ Ǩ ǩ Ǫ ǫ Ǭ ǭ Ǯ ǯ ǰ Ǳ ǲ ǳ Ǵ ǵ Ǻ ǻ Ǽ ǽ Ǿ ǿ Ȁ ȁ Ȃ ȃ"
#define UNICODE_IPA_EXTENSIONS "ɐ ɑ ɒ ɓ ɔ ɕ ɖ ɗ ɘ ə ɚ ɛ ɜ ɝ ɞ ɟ ɠ ɡ ɢ ɣ ɤ ɥ ɦ ɧ ɨ ɩ ɪ ɫ ɬ ɭ ɮ ɯ ɰ ɱ ɲ ɳ ɴ ɵ ɶ ɷ ɸ ɹ ɺ ɻ ɼ ɽ ɾ ɿ ʀ ʁ ʂ ʃ ʄ ʅ ʆ ʇ ʈ ʉ ʊ ʋ ʌ ʍ ʎ ʏ ʐ ʑ ʒ ʓ ʔ ʕ ʖ ʗ ʘ ʙ ʚ ʛ ʜ ʝ ʞ ʟ ʠ ʡ ʢ ʣ ʤ ʥ ʦ ʧ ʨ"
#define UNICODE_SPACING_MODIFIERS "ʰ ʱ ʲ ʳ ʴ ʵ ʶ ʷ ʸ ʹ ʺ ʻ ʼ ʽ ʾ ʿ ˀ ˁ ˂ ˃ ˄ ˅ ˆ ˇ ˈ ˉ ˊ ˋ ˌ ˍ ˎ ˏ ː ˑ ˒ ˓ ˔ ˕ ˖ ˗ ˘ ˙ ˚ ˛ ˜ ˝ ˞ ˠ ˡ ˢ ˣ ˤ ˥ ˦ ˧ ˨ ˩"
#define UNICODE_DIACRITICAL_MARKS "̀ ́ ̂ ̃ ̄ ̅ ̆ ̇ ̈ ̉ ̊ ̋ ̌ ̍ ̎ ̏ ̐ ̑ ̒ ̓ ̔ ̕ ̖ ̗ ̘ ̙ ̚ ̛ ̜ ̝ ̞ ̟ ̠ ̡ ̢ ̣ ̤ ̥ ̦ ̧ ̨ ̩ ̪ ̫ ̬ ̭ ̮ ̯ ̰ ̱ ̲ ̳ ̴ ̵ ̶ ̷ ̸ ̹ ̺ ̻ ̼ ̽ ̾ ̿ ̀ ́ ͂ ̓ ̈́ ͅ ͠ ͡"
#define UNICODE_GREEK "ʹ ͵ ͺ ; ΄ ΅ Ά · Έ Ή Ί Ό Ύ Ώ ΐ Α Β Γ Δ Ε Ζ Η Θ Ι Κ Λ Μ Ν Ξ Ο Π Ρ Σ Τ Υ Φ Χ Ψ Ω Ϊ Ϋ ά έ ή ί ΰ α β γ δ ε ζ η θ ι κ λ μ ν ξ ο π ρ ς σ τ υ φ χ ψ ω ϊ ϋ ό ύ ώ ϐ ϑ ϒ ϓ ϔ ϕ ϖ Ϛ Ϝ Ϟ Ϡ Ϣ ϣ Ϥ ϥ Ϧ ϧ Ϩ ϩ Ϫ ϫ Ϭ ϭ Ϯ ϯ ϰ ϱ ϲ ϳ"
#define UNICODE_CYRILLIC "Ё Ђ Ѓ Є Ѕ І Ї Ј Љ Њ Ћ Ќ Ў Џ А Б В Г Д Е Ж З И Й К Л М Н О П Р С Т У Ф Х Ц Ч Ш Щ Ъ Ы Ь Э Ю Я а б в г д е ж з и й к л м н о п р с т у ф х ц ч ш щ ъ ы ь э ю я ё ђ ѓ є ѕ і ї ј љ њ ћ ќ ў џ Ѡ ѡ Ѣ ѣ Ѥ ѥ Ѧ ѧ Ѩ ѩ Ѫ ѫ Ѭ ѭ Ѯ ѯ Ѱ ѱ Ѳ ѳ Ѵ ѵ Ѷ ѷ Ѹ ѹ Ѻ ѻ Ѽ ѽ Ѿ ѿ Ҁ ҁ ҂ ҃ "
#define UNICODE_ARMENIAN "Ա Բ Գ Դ Ե Զ Է Ը Թ Ժ Ի Լ Խ Ծ Կ Հ Ձ Ղ Ճ Մ Յ Ն Շ Ո Չ Պ Ջ Ռ Ս Վ Տ Ր Ց Ւ Փ Ք Օ Ֆ ՙ ՚ ՛ ՜ ՝ ՞ ՟ ա բ գ դ ե զ է ը թ ժ ի լ խ ծ կ հ ձ ղ ճ մ յ ն շ ո չ պ ջ ռ ս վ տ ր ց ւ փ ք օ ֆ և ։"
#define UNICODE_HEBREW "֑ ֒ ֓ ֔ ֕ ֖ ֗ ֘ ֙ ֚ ֛ ֜ ֝ ֞ ֟ ֠ ֡ ֣ ֤ ֥ ֦ ֧ ֨ ֩ ֪ ֫ ֬ ֭ ֮ ֯ ְ ֱ ֲ ֳ ִ ֵ ֶ ַ ָ ֹ ֻ ּ ֽ ־ ֿ ׀ ׁ ׂ ׃ ׄ א ב ג ד ה ו ז ח ט י ך כ ל ם מ ן נ ס ע ף פ ץ צ ק ר ש ת װ ױ ײ ׳ ״"
#define UNICODE_ARABIC "، ؛ ؟ ء آ أ ؤ إ ئ ا ب ة ت ث ج ح خ د ذ ر ز س ش ص ض ط ظ ع غ ـ ف ق ك ل م ن ه و ى ي ً ٌ ٍ َ ُ ِ ّ ْ ٠ ١ ٢ ٣ ٤ ٥ ٦ ٧ ٨ ٩ ٪ ٫ ٬ ٭ ٰ ٱ ٲ ٳ ٴ ٵ ٶ ٷ ٸ ٹ ٺ ٻ ټ ٽ پ ٿ ڀ ځ ڂ ڃ ڄ څ چ ڇ ڈ ډ ڊ ڋ ڌ ڍ ڎ ڏ ڐ ڑ ڒ ړ ڔ ڕ ږ ڗ ژ ڙ ښ ڛ ڜ ڝ ڞ ڟ ڠ ڡ ڢ ڣ ڤ ڥ ڦ ڧ ڨ ک ڪ ګ ڬ ڭ ڮ گ ڰ ڱ"
#define UNICODE_DEVANAGARI "ँ ं ः अ आ इ ई उ ऊ ऋ ऌ ऍ ऎ ए ऐ ऑ ऒ ओ औ क ख ग घ ङ च छ ज झ ञ ट ठ ड ढ ण त थ द ध न ऩ प फ ब भ म य र ऱ ल ळ ऴ व श ष स ह ़ ऽ ा ि ी ु ू ृ ॄ ॅ ॆ े ै ॉ ॊ ो ौ ् ॐ ॑ ॒ ॓ ॔ क़ ख़ ग़ ज़ ड़ ढ़ फ़ य़ ॠ ॡ ॢ ॣ । ॥ ० १ २ ३ ४ ५ ६ ७ ८ ९ ॰"
#define UNICODE_BENGALI "ঁ ং ঃ অ আ ই ঈ উ ঊ ঋ ঌ এ ঐ ও ঔ ক খ গ ঘ ঙ চ ছ জ ঝ ঞ ট ঠ ড ঢ ণ ত থ দ ধ ন প ফ ব ভ ম য র ল শ ষ স হ ় া ি ী ু ূ ৃ ৄ ে ৈ ো ৌ ্ ৗ ড় ঢ় য় ৠ ৡ ৢ ৣ ০ ১ ২ ৩ ৪ ৫ ৬ ৭ ৮ ৯ ৰ ৱ ৲ ৳ ৴ ৵ ৶ ৷ ৸ ৹ ৺"
#define UNICODE_GURMUKHHI "ਂ ਅ ਆ ਇ ਈ ਉ ਊ ਏ ਐ ਓ ਔ ਕ ਖ ਗ ਘ ਙ ਚ ਛ ਜ ਝ ਞ ਟ ਠ ਡ ਢ ਣ ਤ ਥ ਦ ਧ ਨ ਪ ਫ ਬ ਭ ਮ ਯ ਰ ਲ ਲ਼ ਵ ਸ਼ ਸ ਹ ਼ ਾ ਿ ੀ ੁ ੂ ੇ ੈ ੋ ੌ ੍ ਖ਼ ਗ਼ ਜ਼ ੜ ਫ਼ ੦ ੧ ੨ ੩ ੪ ੫ ੬ ੭ ੮ ੯ ੰ ੱ ੲ ੳ ੴ"
#define UNICODE_GUJARATI "ઁ ં ઃ અ આ ઇ ઈ ઉ ઊ ઋ ઍ એ ઐ ઑ ઓ ઔ ક ખ ગ ઘ ઙ ચ છ જ ઝ ઞ ટ ઠ ડ ઢ ણ ત થ દ ધ ન પ ફ બ ભ મ ય ર લ ળ વ શ ષ સ હ ઼ ઽ ા િ ી ુ ૂ ૃ ૄ ૅ ે ૈ ૉ ો ૌ ્ ૐ ૠ ૦ ૧ ૨ ૩ ૪ ૫ ૬ ૭ ૮ ૯"
#define UNICODE_ORIYA "ଁ ଂ ଃ ଅ ଆ ଇ ଈ ଉ ଊ ଋ ଌ ଏ ଐ ଓ ଔ କ ଖ ଗ ଘ ଙ ଚ ଛ ଜ ଝ ଞ ଟ ଠ ଡ ଢ ଣ ତ ଥ ଦ ଧ ନ ପ ଫ ବ ଭ ମ ଯ ର ଲ ଳ ଶ ଷ ସ ହ ଼ ଽ ା ି ୀ ୁ ୂ ୃ େ ୈ ୋ ୌ ୍ ୖ ୗ ଡ଼ ଢ଼ ୟ ୠ ୡ ୦ ୧ ୨ ୩ ୪ ୫ ୬ ୭ ୮ ୯ ୰"
#define UNICODE_TAMIL "ஂ ஃ அ ஆ இ ஈ உ ஊ எ ஏ ஐ ஒ ஓ ஔ க ங ச ஜ ஞ ட ண த ந ன ப ம ய ர ற ல ள ழ வ ஷ ஸ ஹ ா ி ீ ு ூ ெ ே ை ொ ோ ௌ ் ௗ ௧ ௨ ௩ ௪ ௫ ௬ ௭ ௮ ௯ ௰ ௱ ௲"
#define UNICODE_TELUGU "ఁ ం ః అ ఆ ఇ ఈ ఉ ఊ ఋ ఌ ఎ ఏ ఐ ఒ ఓ ఔ క ఖ గ ఘ ఙ చ ఛ జ ఝ ఞ ట ఠ డ ఢ ణ త థ ద ధ న ప ఫ బ భ మ య ర ఱ ల ళ వ శ ష స హ ా ి ీ ు ూ ృ ౄ ె ే ై ొ ో ౌ ్ ౕ ౖ ౠ ౡ ౦ ౧ ౨ ౩ ౪ ౫ ౬ ౭ ౮ ౯"
#define UNICODE_KANNADA "ಂ ಃ ಅ ಆ ಇ ಈ ಉ ಊ ಋ ಌ ಎ ಏ ಐ ಒ ಓ ಔ ಕ ಖ ಗ ಘ ಙ ಚ ಛ ಜ ಝ ಞ ಟ ಠ ಡ ಢ ಣ ತ ಥ ದ ಧ ನ ಪ ಫ ಬ ಭ ಮ ಯ ರ ಱ ಲ ಳ ವ ಶ ಷ ಸ ಹ ಾ ಿ ೀ ು ೂ ೃ ೄ ೆ ೇ ೈ ೊ ೋ ೌ ್ ೕ ೖ ೞ ೠ ೡ ೦ ೧ ೨ ೩ ೪ ೫ ೬ ೭ ೮ ೯"
#define UNICODE_MALAYALAM "ം ഃ അ ആ ഇ ഈ ഉ ഊ ഋ ഌ എ ഏ ഐ ഒ ഓ ഔ ക ഖ ഗ ഘ ങ ച ഛ ജ ഝ ഞ ട ഠ ഡ ഢ ണ ത ഥ ദ ധ ന പ ഫ ബ ഭ മ യ ര റ ല ള ഴ വ ശ ഷ സ ഹ ാ ി ീ ു ൂ ൃ െ േ ൈ ൊ ോ ൌ ് ൗ ൠ ൡ ൦ ൧ ൨ ൩ ൪ ൫ ൬ ൭ ൮ ൯"
#define UNICODE_THAI "ก ข ฃ ค ฅ ฆ ง จ ฉ ช ซ ฌ ญ ฎ ฏ ฐ ฑ ฒ ณ ด ต ถ ท ธ น บ ป ผ ฝ พ ฟ ภ ม ย ร ฤ ล ฦ ว ศ ษ ส ห ฬ อ ฮ ฯ ะ ั า ำ ิ ี ึ ื ุ ู ฺ ฿ เ แ โ ใ ไ ๅ ๆ ็ ่ ้ ๊ ๋ ์ ํ ๎ ๏ ๐ ๑ ๒ ๓ ๔ ๕ ๖ ๗ ๘ ๙ ๚ ๛"
#define UNICODE_LAO "ກ ຂ ຄ ງ ຈ ຊ ຍ ດ ຕ ຖ ທ ນ ບ ປ ຜ ຝ ພ ຟ ມ ຢ ຣ ລ ວ ສ ຫ ອ ຮ ຯ ະ ັ າ ຳ ິ ີ ຶ ື ຸ ູ ົ ຼ ຽ ເ ແ ໂ ໃ ໄ ໆ ່ ້ ໊ ໋ ໌ ໍ ໐ ໑ ໒ ໓ ໔ ໕ ໖ ໗ ໘ ໙ ໜ ໝ"
#define UNICODE_TIBETAN "ༀ ༁ ༂ ༃ ༄ ༅ ༆ ༇ ༈ ༉ ༊ ་ ༌ ། ༎ ༏ ༐ ༑ ༒ ༓ ༔ ༕ ༖ ༗ ༘ ༙ ༚ ༛ ༜ ༝ ༞ ༟ ༠ ༡ ༢ ༣ ༤ ༥ ༦ ༧ ༨ ༩ ༪ ༫ ༬ ༭ ༮ ༯ ༰ ༱ ༲ ༳ ༴ ༵ ༶ ༷ ༸ ༹ ༺ ༻ ༼ ༽ ༾ ༿ ཀ ཁ ག གྷ ང ཅ ཆ ཇ ཉ ཊ ཋ ཌ ཌྷ ཎ ཏ ཐ ད དྷ ན པ ཕ བ བྷ མ ཙ ཚ ཛ ཛྷ ཝ ཞ ཟ འ ཡ ར ལ ཤ ཥ ས ཧ ཨ ཀྵ ཱ ི ཱི ུ ཱུ ྲྀ ཷ ླྀ ཹ ེ ཻ ོ ཽ ཾ ཿ ྀ ཱྀ ྂ ྃ ྄ ྅ ྆ ྇"
#define UNICODE_GEORGIAN "Ⴀ Ⴁ Ⴂ Ⴃ Ⴄ Ⴅ Ⴆ Ⴇ Ⴈ Ⴉ Ⴊ Ⴋ Ⴌ Ⴍ Ⴎ Ⴏ Ⴐ Ⴑ Ⴒ Ⴓ Ⴔ Ⴕ Ⴖ Ⴗ Ⴘ Ⴙ Ⴚ Ⴛ Ⴜ Ⴝ Ⴞ Ⴟ Ⴠ Ⴡ Ⴢ Ⴣ Ⴤ Ⴥ ა ბ გ დ ე ვ ზ თ ი კ ლ მ ნ ო პ ჟ რ ს ტ უ ფ ქ ღ ყ შ ჩ ც ძ წ ჭ ხ ჯ ჰ ჱ ჲ ჳ ჴ ჵ ჶ ჻"
#define UNICODE_HANGUL_JAMO "ᄀ ᄁ ᄂ ᄃ ᄄ ᄅ ᄆ ᄇ ᄈ ᄉ ᄊ ᄋ ᄌ ᄍ ᄎ ᄏ ᄐ ᄑ ᄒ ᄓ ᄔ ᄕ ᄖ ᄗ ᄘ ᄙ ᄚ ᄛ ᄜ ᄝ ᄞ ᄟ ᄠ ᄡ ᄢ ᄣ ᄤ ᄥ ᄦ ᄧ ᄨ ᄩ ᄪ ᄫ ᄬ ᄭ ᄮ ᄯ ᄰ ᄱ ᄲ ᄳ ᄴ ᄵ ᄶ ᄷ ᄸ ᄹ ᄺ ᄻ ᄼ ᄽ ᄾ ᄿ ᅀ ᅁ ᅂ ᅃ ᅄ ᅅ ᅆ ᅇ ᅈ ᅉ ᅊ ᅋ ᅌ ᅍ ᅎ ᅏ ᅐ ᅑ ᅒ ᅓ ᅔ ᅕ ᅖ ᅗ ᅘ ᅙ ᅟ ᅠ ᅡ ᅢ ᅣ ᅤ ᅥ ᅦ ᅧ ᅨ ᅩ ᅪ ᅫ ᅬ ᅭ ᅮ ᅯ ᅰ ᅱ ᅲ ᅳ ᅴ ᅵ ᅶ ᅷ ᅸ ᅹ ᅺ ᅻ ᅼ ᅽ ᅾ ᅿ ᆀ ᆁ ᆂ ᆃ ᆄ"
#define UNICODE_LATIN_EXTENDED_ADDITIONAL "Ḁ ḁ Ḃ ḃ Ḅ ḅ Ḇ ḇ Ḉ ḉ Ḋ ḋ Ḍ ḍ Ḏ ḏ Ḑ ḑ Ḓ ḓ Ḕ ḕ Ḗ ḗ Ḙ ḙ Ḛ ḛ Ḝ ḝ Ḟ ḟ Ḡ ḡ Ḣ ḣ Ḥ ḥ Ḧ ḧ Ḩ ḩ Ḫ ḫ Ḭ ḭ Ḯ ḯ Ḱ ḱ Ḳ ḳ Ḵ ḵ Ḷ ḷ Ḹ ḹ Ḻ ḻ Ḽ ḽ Ḿ ḿ Ṁ ṁ Ṃ ṃ Ṅ ṅ Ṇ ṇ Ṉ ṉ Ṋ ṋ Ṍ ṍ Ṏ ṏ Ṑ ṑ Ṓ ṓ Ṕ ṕ Ṗ ṗ Ṙ ṙ Ṛ ṛ Ṝ ṝ Ṟ ṟ Ṡ ṡ Ṣ ṣ Ṥ ṥ Ṧ ṧ Ṩ ṩ Ṫ ṫ Ṭ ṭ Ṯ ṯ Ṱ ṱ Ṳ ṳ Ṵ ṵ Ṷ ṷ Ṹ ṹ Ṻ ṻ Ṽ ṽ Ṿ ṿ"
#define UNICODE_GREEK_EXTENDED "ἀ ἁ ἂ ἃ ἄ ἅ ἆ ἇ Ἀ Ἁ Ἂ Ἃ Ἄ Ἅ Ἆ Ἇ ἐ ἑ ἒ ἓ ἔ ἕ Ἐ Ἑ Ἒ Ἓ Ἔ Ἕ ἠ ἡ ἢ ἣ ἤ ἥ ἦ ἧ Ἠ Ἡ Ἢ Ἣ Ἤ Ἥ Ἦ Ἧ ἰ ἱ ἲ ἳ ἴ ἵ ἶ ἷ Ἰ Ἱ Ἲ Ἳ Ἴ Ἵ Ἶ Ἷ ὀ ὁ ὂ ὃ ὄ ὅ Ὀ Ὁ Ὂ Ὃ Ὄ Ὅ ὐ ὑ ὒ ὓ ὔ ὕ ὖ ὗ Ὑ Ὓ Ὕ Ὗ ὠ ὡ ὢ ὣ ὤ ὥ ὦ ὧ Ὠ Ὡ Ὢ Ὣ Ὤ Ὥ Ὦ Ὧ ὰ ά ὲ έ ὴ ή ὶ ί ὸ ό ὺ ύ ὼ ώ ᾀ ᾁ ᾂ ᾃ ᾄ ᾅ ᾆ ᾇ ᾈ ᾉ ᾊ ᾋ ᾌ ᾍ"
#define UNICODE_PUNCTUATION "                      ​ ‌ ‍ ‎ ‏ ‐ ‑ ‒ – — ― ‖ ‗ ‘ ’ ‚ ‛ “ ” „ ‟ † ‡ • ‣ ․ ‥ … ‧     ‪ ‫ ‬ ‭ ‮ ‰ ‱ ′ ″ ‴ ‵ ‶ ‷ ‸ ‹ › ※ ‼ ‽ ‾ ‿ ⁀ ⁁ ⁂ ⁃ ⁄ ⁅ ⁆ ⁪ ⁫ ⁬ ⁭ ⁮ ⁯"
#define UNICODE_SUPERSUB_SCRIPTS "⁰ ⁴ ⁵ ⁶ ⁷ ⁸ ⁹ ⁺ ⁻ ⁼ ⁽ ⁾ ⁿ ₀ ₁ ₂ ₃ ₄ ₅ ₆ ₇ ₈ ₉ ₊ ₋ ₌ ₍ ₎"
#define UNICODE_CURRENCY "₠ ₡ ₢ ₣ ₤ ₥ ₦ ₧ ₨ ₩ ₪ ₫"
#define UNICODE_COMBINING_MARKS "⃐ ⃑ ⃒ ⃓ ⃔ ⃕ ⃖ ⃗ ⃘ ⃙ ⃚ ⃛ ⃜ ⃝ ⃞ ⃟ ⃠ ⃡"
#define UNICODE_LETTERLIKE "℀ ℁ ℂ ℃ ℄ ℅ ℆ ℇ ℈ ℉ ℊ ℋ ℌ ℍ ℎ ℏ ℐ ℑ ℒ ℓ ℔ ℕ № ℗ ℘ ℙ ℚ ℛ ℜ ℝ ℞ ℟ ℠ ℡ ™ ℣ ℤ ℥ Ω ℧ ℨ ℩ K Å ℬ ℭ ℮ ℯ ℰ ℱ Ⅎ ℳ ℴ ℵ ℶ ℷ ℸ"
#define UNICODE_NUMBER_FORMS "⅓ ⅔ ⅕ ⅖ ⅗ ⅘ ⅙ ⅚ ⅛ ⅜ ⅝ ⅞ ⅟ Ⅰ Ⅱ Ⅲ Ⅳ Ⅴ Ⅵ Ⅶ Ⅷ Ⅸ Ⅹ Ⅺ Ⅻ Ⅼ Ⅽ Ⅾ Ⅿ ⅰ ⅱ ⅲ ⅳ ⅴ ⅵ ⅶ ⅷ ⅸ ⅹ ⅺ ⅻ ⅼ ⅽ ⅾ ⅿ ↀ ↁ ↂ"
#define UNICODE_ARROWS "← ↑ → ↓ ↔ ↕ ↖ ↗ ↘ ↙ ↚ ↛ ↜ ↝ ↞ ↟ ↠ ↡ ↢ ↣ ↤ ↥ ↦ ↧ ↨ ↩ ↪ ↫ ↬ ↭ ↮ ↯ ↰ ↱ ↲ ↳ ↴ ↵ ↶ ↷ ↸ ↹ ↺ ↻ ↼ ↽ ↾ ↿ ⇀ ⇁ ⇂ ⇃ ⇄ ⇅ ⇆ ⇇ ⇈ ⇉ ⇊ ⇋ ⇌ ⇍ ⇎ ⇏ ⇐ ⇑ ⇒ ⇓ ⇔ ⇕ ⇖ ⇗ ⇘ ⇙ ⇚ ⇛ ⇜ ⇝ ⇞ ⇟ ⇠ ⇡ ⇢ ⇣ ⇤ ⇥ ⇦ ⇧ ⇨ ⇩ ⇪"
#define UNICODE_ARROWS_SUPPLEMENT_A "⟰ ⟱ ⟲ ⟳ ⟴ ⟵ ⟶ ⟷ ⟸ ⟹ ⟺ ⟻ ⟼ ⟽ ⟾ ⟿"
#define UNICODE_ARROWS_SUPPLEMENT_B "⤀ ⤁ ⤂ ⤃ ⤄ ⤅ ⤆ ⤇ ⤈ ⤉ ⤊ ⤋ ⤌ ⤍ ⤎ ⤏ ⤐ ⤑ ⤒ ⤓ ⤔ ⤕ ⤖ ⤗ ⤘ ⤙ ⤚ ⤛ ⤜ ⤝ ⤞ ⤟ ⤠ ⤡ ⤢ ⤣ ⤤ ⤥ ⤦ ⤧ ⤨ ⤩ ⤪ ⤫ ⤬ ⤭ ⤮ ⤯ ⤰ ⤱ ⤲ ⤳ ⤴ ⤵ ⤶ ⤷ ⤸ ⤹ ⤺ ⤻ ⤼ ⤽ ⤾ ⤿ ⥀ ⥁ ⥂ ⥃ ⥄ ⥅ ⥆ ⥇ ⥈ ⥉ ⥊ ⥋ ⥌ ⥍ ⥎ ⥏ ⥐ ⥑ ⥒ ⥓ ⥔ ⥕ ⥖ ⥗ ⥘ ⥙ ⥚ ⥛ ⥜ ⥝ ⥞ ⥟ ⥠ ⥡ ⥢ ⥣ ⥤ ⥥ ⥦ ⥧ ⥨ ⥩ ⥪ ⥫ ⥬ ⥭ ⥮ ⥯ ⥰ ⥱ ⥲ ⥳ ⥴ ⥵ ⥶ ⥷ ⥸ ⥹ ⥺ ⥻ ⥼ ⥽ ⥾ ⥿"
#define UNICODE_MATH_OPERATORS "∀ ∁ ∂ ∃ ∄ ∅ ∆ ∇ ∈ ∉ ∊ ∋ ∌ ∍ ∎ ∏ ∐ ∑ − ∓ ∔ ∕ ∖ ∗ ∘ ∙ √ ∛ ∜ ∝ ∞ ∟ ∠ ∡ ∢ ∣ ∤ ∥ ∦ ∧ ∨ ∩ ∪ ∫ ∬ ∭ ∮ ∯ ∰ ∱ ∲ ∳ ∴ ∵ ∶ ∷ ∸ ∹ ∺ ∻ ∼ ∽ ∾ ∿ ≀ ≁ ≂ ≃ ≄ ≅ ≆ ≇ ≈ ≉ ≊ ≋ ≌ ≍ ≎ ≏ ≐ ≑ ≒ ≓ ≔ ≕ ≖ ≗ ≘ ≙ ≚ ≛ ≜ ≝ ≞ ≟ ≠ ≡ ≢ ≣ ≤ ≥ ≦ ≧ ≨ ≩ ≪ ≫ ≬ ≭ ≮ ≯ ≰ ≱ ≲ ≳ ≴ ≵ ≶ ≷ ≸ ≹ ≺ ≻ ≼ ≽ ≾ ≿"
#define UNICODE_MATH_OPERATORS_SUPPLEMENT "⨀ ⨁ ⨂ ⨃ ⨄ ⨅ ⨆ ⨇ ⨈ ⨉ ⨊ ⨋ ⨌ ⨍ ⨎ ⨏ ⨐ ⨑ ⨒ ⨓ ⨔ ⨕ ⨖ ⨗ ⨘ ⨙ ⨚ ⨛ ⨜ ⨝ ⨞ ⨟ ⨠ ⨡ ⨢ ⨣ ⨤ ⨥ ⨦ ⨧ ⨨ ⨩ ⨪ ⨫ ⨬ ⨭ ⨮ ⨯ ⨰ ⨱ ⨲ ⨳ ⨴ ⨵ ⨶ ⨷ ⨸ ⨹ ⨺ ⨻ ⨼ ⨽ ⨾ ⨿ ⩀ ⩁ ⩂ ⩃ ⩄ ⩅ ⩆ ⩇ ⩈ ⩉ ⩊ ⩋ ⩌ ⩍ ⩎ ⩏ ⩐ ⩑ ⩒ ⩓ ⩔ ⩕ ⩖ ⩗ ⩘ ⩙ ⩚ ⩛ ⩜ ⩝ ⩞ ⩟ ⩠ ⩡ ⩢ ⩣ ⩤ ⩥ ⩦ ⩧ ⩨ ⩩ ⩪ ⩫ ⩬ ⩭ ⩮ ⩯ ⩰ ⩱ ⩲ ⩳ ⩴ ⩵ ⩶ ⩷ ⩸ ⩹ ⩺ ⩻ ⩼ ⩽ ⩾ ⩿"
#define UNICODE_MATH_MISC_A "⟐ ⟑ ⟒ ⟓ ⟔ ⟕ ⟖ ⟗ ⟘ ⟙ ⟚ ⟛ ⟜ ⟝ ⟞ ⟟ ⟠ ⟡ ⟢ ⟣ ⟤ ⟥ ⟦ ⟧ ⟨ ⟩ ⟪ ⟫"
#define UNICODE_CONTROL_PICTURES "␀ ␁ ␂ ␃ ␄ ␅ ␆ ␇ ␈ ␉ ␊ ␋ ␌ ␍ ␎ ␏ ␐ ␑ ␒ ␓ ␔ ␕ ␖ ␗ ␘ ␙ ␚ ␛ ␜ ␝ ␞ ␟ ␠ ␡ ␢ ␣ ␤"
#define UNICODE_OPTICAL_CHARACTERS "⑀ ⑁ ⑂ ⑃ ⑄ ⑅ ⑆ ⑇ ⑈ ⑉ ⑊"
#define UNICODE_ENCLOSED_ALPHANUMERICS "① ② ③ ④ ⑤ ⑥ ⑦ ⑧ ⑨ ⑩ ⑪ ⑫ ⑬ ⑭ ⑮ ⑯ ⑰ ⑱ ⑲ ⑳ ⑴ ⑵ ⑶ ⑷ ⑸ ⑹ ⑺ ⑻ ⑼ ⑽ ⑾ ⑿ ⒀ ⒁ ⒂ ⒃ ⒄ ⒅ ⒆ ⒇ ⒈ ⒉ ⒊ ⒋ ⒌ ⒍ ⒎ ⒏ ⒐ ⒑ ⒒ ⒓ ⒔ ⒕ ⒖ ⒗ ⒘ ⒙ ⒚ ⒛ ⒜ ⒝ ⒞ ⒟ ⒠ ⒡ ⒢ ⒣ ⒤ ⒥ ⒦ ⒧ ⒨ ⒩ ⒪ ⒫ ⒬ ⒭ ⒮ ⒯ ⒰ ⒱ ⒲ ⒳ ⒴ ⒵ Ⓐ Ⓑ Ⓒ Ⓓ Ⓔ Ⓕ Ⓖ Ⓗ Ⓘ Ⓙ Ⓚ Ⓛ Ⓜ Ⓝ Ⓞ Ⓟ Ⓠ Ⓡ Ⓢ Ⓣ Ⓤ Ⓥ Ⓦ Ⓧ Ⓨ Ⓩ ⓐ ⓑ ⓒ ⓓ ⓔ ⓕ ⓖ ⓗ ⓘ ⓙ ⓚ ⓛ ⓜ ⓝ ⓞ ⓟ"
#define UNICODE_BOX_DRAWING "─ ━ │ ┃ ┄ ┅ ┆ ┇ ┈ ┉ ┊ ┋ ┌ ┍ ┎ ┏ ┐ ┑ ┒ ┓ └ ┕ ┖ ┗ ┘ ┙ ┚ ┛ ├ ┝ ┞ ┟ ┠ ┡ ┢ ┣ ┤ ┥ ┦ ┧ ┨ ┩ ┪ ┫ ┬ ┭ ┮ ┯ ┰ ┱ ┲ ┳ ┴ ┵ ┶ ┷ ┸ ┹ ┺ ┻ ┼ ┽ ┾ ┿ ╀ ╁ ╂ ╃ ╄ ╅ ╆ ╇ ╈ ╉ ╊ ╋ ╌ ╍ ╎ ╏ ═ ║ ╒ ╓ ╔ ╕ ╖ ╗ ╘ ╙ ╚ ╛ ╜ ╝ ╞ ╟ ╠ ╡ ╢ ╣ ╤ ╥ ╦ ╧ ╨ ╩ ╪ ╫ ╬ ╭ ╮ ╯ ╰ ╱ ╲ ╳ ╴ ╵ ╶ ╷ ╸ ╹ ╺ ╻ ╼ ╽ ╾ ╿"
#define UNICODE_BLOCK_ELEMENTS "▀ ▁ ▂ ▃ ▄ ▅ ▆ ▇ █ ▉ ▊ ▋ ▌ ▍ ▎ ▏ ▐ ░ ▒ ▓ ▔ ▕"
#define UNICODE_GEOMETRIC_SHAPES "■ □ ▢ ▣ ▤ ▥ ▦ ▧ ▨ ▩ ▪ ▫ ▬ ▭ ▮ ▯ ▰ ▱ ▲ △ ▴ ▵ ▶ ▷ ▸ ▹ ► ▻ ▼ ▽ ▾ ▿ ◀ ◁ ◂ ◃ ◄ ◅ ◆ ◇ ◈ ◉ ◊ ○ ◌ ◍ ◎ ● ◐ ◑ ◒ ◓ ◔ ◕ ◖ ◗ ◘ ◙ ◚ ◛ ◜ ◝ ◞ ◟ ◠ ◡ ◢ ◣ ◤ ◥ ◦ ◧ ◨ ◩ ◪ ◫ ◬ ◭ ◮ ◯"
#define UNICODE_MISC_SYMBOLS "☀ ☁ ☂ ☃ ☄ ★ ☆ ☇ ☈ ☉ ☊ ☋ ☌ ☍ ☎ ☏ ☐ ☑ ☒ ☓ ☚ ☛ ☜ ☝ ☞ ☟ ☠ ☡ ☢ ☣ ☤ ☥ ☦ ☧ ☨ ☩ ☪ ☫ ☬ ☭ ☮ ☯ ☰ ☱ ☲ ☳ ☴ ☵ ☶ ☷ ☸ ☹ ☺ ☻ ☼ ☽ ☾ ☿ ♀ ♁ ♂ ♃ ♄ ♅ ♆ ♇ ♈ ♉ ♊ ♋ ♌ ♍ ♎ ♏ ♐ ♑ ♒ ♓ ♔ ♕ ♖ ♗ ♘ ♙ ♚ ♛ ♜ ♝ ♞ ♟ ♠ ♡ ♢ ♣ ♤ ♥ ♦ ♧ ♨ ♩ ♪ ♫ ♬ ♭ ♮ ♯"
#define UNICODE_DINGBATS "✁ ✂ ✃ ✄ ✆ ✇ ✈ ✉ ✌ ✍ ✎ ✏ ✐ ✑ ✒ ✓ ✔ ✕ ✖ ✗ ✘ ✙ ✚ ✛ ✜ ✝ ✞ ✟ ✠ ✡ ✢ ✣ ✤ ✥ ✦ ✧ ✩ ✪ ✫ ✬ ✭ ✮ ✯ ✰ ✱ ✲ ✳ ✴ ✵ ✶ ✷ ✸ ✹ ✺ ✻ ✼ ✽ ✾ ✿ ❀ ❁ ❂ ❃ ❄ ❅ ❆ ❇ ❈ ❉ ❊ ❋ ❍ ❏ ❐ ❑ ❒ ❖ ❘ ❙ ❚ ❛ ❜ ❝ ❞ ❡ ❢ ❣ ❤ ❥ ❦ ❧ ❶ ❷ ❸ ❹ ❺ ❻ ❼ ❽ ❾ ❿ ➀ ➁ ➂ ➃ ➄ ➅ ➆ ➇ ➈ ➉ ➊ ➋ ➌ ➍ ➎ ➏ ➐ ➑ ➒ ➓ ➔ ➘ ➙ ➚ ➛ ➜ ➝"
#define UNICODE_CJK "　 、 。 〃 〄 々 〆 〇 〈 〉 《 》 「 」 『 』 【 】 〒 〓 〔 〕 〖 〗 〘 〙 〚 〛 〜 〝 〞 〟 〠 〡 〢 〣 〤 〥 〦 〧 〨 〩 〪 〫 〬 〭 〮 〯 〰 〱 〲 〳 〴 〵 〶 〷 〿"
#define UNICODE_HIRAGANA "ぁ あ ぃ い ぅ う ぇ え ぉ お か が き ぎ く ぐ け げ こ ご さ ざ し じ す ず せ ぜ そ ぞ た だ ち ぢ っ つ づ て で と ど な に ぬ ね の は ば ぱ ひ び ぴ ふ ぶ ぷ へ べ ぺ ほ ぼ ぽ ま み む め も ゃ や ゅ ゆ ょ よ ら り る れ ろ ゎ わ ゐ ゑ を ん ゔ ゙ ゚ ゛ ゜ ゝ ゞ"
#define UNICODE_KATAKANA "ァ ア ィ イ ゥ ウ ェ エ ォ オ カ ガ キ ギ ク グ ケ ゲ コ ゴ サ ザ シ ジ ス ズ セ ゼ ソ ゾ タ ダ チ ヂ ッ ツ ヅ テ デ ト ド ナ ニ ヌ ネ ノ ハ バ パ ヒ ビ ピ フ ブ プ ヘ ベ ペ ホ ボ ポ マ ミ ム メ モ ャ ヤ ュ ユ ョ ヨ ラ リ ル レ ロ ヮ ワ ヰ ヱ ヲ ン ヴ ヵ ヶ ヷ ヸ ヹ ヺ ・ ー ヽ ヾ"
#define UNICODE_BOPOMOFO "ㄅ ㄆ ㄇ ㄈ ㄉ ㄊ ㄋ ㄌ ㄍ ㄎ ㄏ ㄐ ㄑ ㄒ ㄓ ㄔ ㄕ ㄖ ㄗ ㄘ ㄙ ㄚ ㄛ ㄜ ㄝ ㄞ ㄟ ㄠ ㄡ ㄢ ㄣ ㄤ ㄥ ㄦ ㄧ ㄨ ㄩ ㄪ ㄫ ㄬ"
#define UNICODE_HANGUL_COMPATIBILITY_JAMO "ㄱ ㄲ ㄳ ㄴ ㄵ ㄶ ㄷ ㄸ ㄹ ㄺ ㄻ ㄼ ㄽ ㄾ ㄿ ㅀ ㅁ ㅂ ㅃ ㅄ ㅅ ㅆ ㅇ ㅈ ㅉ ㅊ ㅋ ㅌ ㅍ ㅎ ㅏ ㅐ ㅑ ㅒ ㅓ ㅔ ㅕ ㅖ ㅗ ㅘ ㅙ ㅚ ㅛ ㅜ ㅝ ㅞ ㅟ ㅠ ㅡ ㅢ ㅣ ㅤ ㅥ ㅦ ㅧ ㅨ ㅩ ㅪ ㅫ ㅬ ㅭ ㅮ ㅯ ㅰ ㅱ ㅲ ㅳ ㅴ ㅵ ㅶ ㅷ ㅸ ㅹ ㅺ ㅻ ㅼ ㅽ ㅾ ㅿ ㆀ ㆁ ㆂ ㆃ ㆄ ㆅ ㆆ ㆇ ㆈ ㆉ ㆊ ㆋ ㆌ ㆍ ㆎ"
#define UNICODE_KANBUN "㆐ ㆑ ㆒ ㆓ ㆔ ㆕ ㆖ ㆗ ㆘ ㆙ ㆚ ㆛ ㆜ ㆝ ㆞ ㆟"
#define UNICODE_CJK_ENCLOSED "㈀ ㈁ ㈂ ㈃ ㈄ ㈅ ㈆ ㈇ ㈈ ㈉ ㈊ ㈋ ㈌ ㈍ ㈎ ㈏ ㈐ ㈑ ㈒ ㈓ ㈔ ㈕ ㈖ ㈗ ㈘ ㈙ ㈚ ㈛ ㈜ ㈠ ㈡ ㈢ ㈣ ㈤ ㈥ ㈦ ㈧ ㈨ ㈩ ㈪ ㈫ ㈬ ㈭ ㈮ ㈯ ㈰ ㈱ ㈲ ㈳ ㈴ ㈵ ㈶ ㈷ ㈸ ㈹ ㈺ ㈻ ㈼ ㈽ ㈾ ㈿ ㉀ ㉁ ㉂ ㉃ ㉠ ㉡ ㉢ ㉣ ㉤ ㉥ ㉦ ㉧ ㉨ ㉩ ㉪ ㉫ ㉬ ㉭ ㉮ ㉯ ㉰ ㉱ ㉲ ㉳ ㉴ ㉵ ㉶ ㉷ ㉸ ㉹ ㉺ ㉻ ㉿ ㊀ ㊁ ㊂ ㊃ ㊄ ㊅ ㊆ ㊇ ㊈ ㊉ ㊊ ㊋ ㊌ ㊍ ㊎ ㊏ ㊐ ㊑ ㊒ ㊓ ㊔ ㊕ ㊖ ㊗ ㊘ ㊙ ㊚ ㊛ ㊜ ㊝ ㊞ ㊟ ㊠ ㊡"
#define UNICODE_CJK_COMPATIBILITY "㌀ ㌁ ㌂ ㌃ ㌄ ㌅ ㌆ ㌇ ㌈ ㌉ ㌊ ㌋ ㌌ ㌍ ㌎ ㌏ ㌐ ㌑ ㌒ ㌓ ㌔ ㌕ ㌖ ㌗ ㌘ ㌙ ㌚ ㌛ ㌜ ㌝ ㌞ ㌟ ㌠ ㌡ ㌢ ㌣ ㌤ ㌥ ㌦ ㌧ ㌨ ㌩ ㌪ ㌫ ㌬ ㌭ ㌮ ㌯ ㌰ ㌱ ㌲ ㌳ ㌴ ㌵ ㌶ ㌷ ㌸ ㌹ ㌺ ㌻ ㌼ ㌽ ㌾ ㌿ ㍀ ㍁ ㍂ ㍃ ㍄ ㍅ ㍆ ㍇ ㍈ ㍉ ㍊ ㍋ ㍌ ㍍ ㍎ ㍏ ㍐ ㍑ ㍒ ㍓ ㍔ ㍕ ㍖ ㍗ ㍘ ㍙ ㍚ ㍛ ㍜ ㍝ ㍞ ㍟ ㍠ ㍡ ㍢ ㍣ ㍤ ㍥ ㍦ ㍧ ㍨ ㍩ ㍪ ㍫ ㍬ ㍭ ㍮ ㍯ ㍰ ㍱ ㍲ ㍳ ㍴ ㍵ ㍶ ㍻ ㍼ ㍽ ㍾ ㍿ ㎀ ㎁ ㎂ ㎃"
#define UNICODE_CJK_UNIFIED_IDEOGRAPHS "一 丁 丂 七 丄 丅 丆 万 丈 三 上 下 丌 不 与 丏 丐 丑 丒 专 且 丕 世 丗 丘 丙 业 丛 东 丝 丞 丟 丠 両 丢 丣 两 严 並 丧 丨 丩 个 丫 丬 中 丮 丯 丰 丱 串 丳 临 丵 丶 丷 丸 丹 为 主 丼 丽 举 丿 乀 乁 乂 乃 乄 久 乆 乇 么 义 乊 之 乌 乍 乎 乏 乐 乑 乒 乓 乔 乕 乖 乗 乘 乙 乚 乛 乜 九 乞 也 习 乡 乢 乣 乤 乥 书 乧 乨 乩 乪 乫 乬 乭 乮 乯 买 乱 乲 乳 乴 乵 乶 乷 乸 乹 乺 乻 乼 乽 乾 乿"
#define UNICODE_HANGUL_SYLLABLES "가 각 갂 갃 간 갅 갆 갇 갈 갉 갊 갋 갌 갍 갎 갏 감 갑 값 갓 갔 강 갖 갗 갘 같 갚 갛 개 객 갞 갟 갠 갡 갢 갣 갤 갥 갦 갧 갨 갩 갪 갫 갬 갭 갮 갯 갰 갱 갲 갳 갴 갵 갶 갷 갸 갹 갺 갻 갼 갽 갾 갿 걀 걁 걂 걃 걄 걅 걆 걇 걈 걉 걊 걋 걌 걍 걎 걏 걐 걑 걒 걓 걔 걕 걖 걗 걘 걙 걚 걛 걜 걝 걞 걟 걠 걡 걢 걣 걤 걥 걦 걧 걨 걩 걪 걫 걬 걭 걮 걯 거 걱 걲 걳 건 걵 걶 걷 걸 걹 걺 걻 걼 걽 걾 걿"
#define UNICODE_PRIVATE_USE "                                                                                                                               "
#define UNICODE_CJK_COMPATIBILITY_IDEOGRAPHS "豈 更 車 賈 滑 串 句 龜 龜 契 金 喇 奈 懶 癩 羅 蘿 螺 裸 邏 樂 洛 烙 珞 落 酪 駱 亂 卵 欄 爛 蘭 鸞 嵐 濫 藍 襤 拉 臘 蠟 廊 朗 浪 狼 郎 來 冷 勞 擄 櫓 爐 盧 老 蘆 虜 路 露 魯 鷺 碌 祿 綠 菉 錄 鹿 論 壟 弄 籠 聾 牢 磊 賂 雷 壘 屢 樓 淚 漏 累 縷 陋 勒 肋 凜 凌 稜 綾 菱 陵 讀 拏 樂 諾 丹 寧 怒 率 異 北 磻 便 復 不 泌 數 索 參 塞 省 葉 說 殺 辰 沈 拾 若 掠 略 亮 兩 凉 梁 糧 良 諒 量 勵"
#define UNICODE_ALPHABETIC_PRESENTATION_FORMS "ﬀ ﬁ ﬂ ﬃ ﬄ ﬅ ﬆ ﬓ ﬔ ﬕ ﬖ ﬗ ﬞ ײַ ﬠ ﬡ ﬢ ﬣ ﬤ ﬥ ﬦ ﬧ ﬨ ﬩ שׁ שׂ שּׁ שּׂ אַ אָ אּ בּ גּ דּ הּ וּ זּ טּ יּ ךּ כּ לּ מּ נּ סּ ףּ פּ צּ קּ רּ שּ תּ וֹ בֿ כֿ פֿ ﭏ"
#define UNICODE_ARABIC_PRESENTATION_FORMS_A "ﭐ ﭑ ﭒ ﭓ ﭔ ﭕ ﭖ ﭗ ﭘ ﭙ ﭚ ﭛ ﭜ ﭝ ﭞ ﭟ ﭠ ﭡ ﭢ ﭣ ﭤ ﭥ ﭦ ﭧ ﭨ ﭩ ﭪ ﭫ ﭬ ﭭ ﭮ ﭯ ﭰ ﭱ ﭲ ﭳ ﭴ ﭵ ﭶ ﭷ ﭸ ﭹ ﭺ ﭻ ﭼ ﭽ ﭾ ﭿ ﮀ ﮁ ﮂ ﮃ ﮄ ﮅ ﮆ ﮇ ﮈ ﮉ ﮊ ﮋ ﮌ ﮍ ﮎ ﮏ ﮐ ﮑ ﮒ ﮓ ﮔ ﮕ ﮖ ﮗ ﮘ ﮙ ﮚ ﮛ ﮜ ﮝ ﮞ ﮟ ﮠ ﮡ ﮢ ﮣ ﮤ ﮥ ﮦ ﮧ ﮨ ﮩ ﮪ ﮫ ﮬ ﮭ ﮮ ﮯ ﮰ ﮱ ﯓ ﯔ ﯕ ﯖ ﯗ ﯘ ﯙ ﯚ ﯛ ﯜ ﯝ ﯞ ﯟ ﯠ ﯡ ﯢ ﯣ ﯤ ﯥ ﯦ ﯧ ﯨ ﯩ ﯪ ﯫ ﯬ ﯭ ﯮ ﯯ ﯰ"
#define UNICODE_COMBINING_HALF_MARKS "︠ ︡ ︢ ︣"
#define UNICODE_CJK_COMPATIBILITY_FORMS "︰ ︱ ︲ ︳ ︴ ︵ ︶ ︷ ︸ ︹ ︺ ︻ ︼ ︽ ︾ ︿ ﹀ ﹁ ﹂ ﹃ ﹄ ﹉ ﹊ ﹋ ﹌ ﹍ ﹎ ﹏"
#define UNICODE_SMALL_FORM_VARIANTS "﹐ ﹑ ﹒ ﹔ ﹕ ﹖ ﹗ ﹘ ﹙ ﹚ ﹛ ﹜ ﹝ ﹞ ﹟ ﹠ ﹡ ﹢ ﹣ ﹤ ﹥ ﹦ ﹨ ﹩ ﹪ ﹫"
#define UNICODE_ARABIC_PRESENTATION_FORMS_B "ﹰ ﹱ ﹲ ﹴ ﹶ ﹷ ﹸ ﹹ ﹺ ﹻ ﹼ ﹽ ﹾ ﹿ ﺀ ﺁ ﺂ ﺃ ﺄ ﺅ ﺆ ﺇ ﺈ ﺉ ﺊ ﺋ ﺌ ﺍ ﺎ ﺏ ﺐ ﺑ ﺒ ﺓ ﺔ ﺕ ﺖ ﺗ ﺘ ﺙ ﺚ ﺛ ﺜ ﺝ ﺞ ﺟ ﺠ ﺡ ﺢ ﺣ ﺤ ﺥ ﺦ ﺧ ﺨ ﺩ ﺪ ﺫ ﺬ ﺭ ﺮ ﺯ ﺰ ﺱ ﺲ ﺳ ﺴ ﺵ ﺶ ﺷ ﺸ ﺹ ﺺ ﺻ ﺼ ﺽ ﺾ ﺿ ﻀ ﻁ ﻂ ﻃ ﻄ ﻅ ﻆ ﻇ ﻈ ﻉ ﻊ ﻋ ﻌ ﻍ ﻎ ﻏ ﻐ ﻑ ﻒ ﻓ ﻔ ﻕ ﻖ ﻗ ﻘ ﻙ ﻚ ﻛ ﻜ ﻝ ﻞ ﻟ ﻠ ﻡ ﻢ ﻣ ﻤ ﻥ ﻦ ﻧ ﻨ ﻩ ﻪ ﻫ ﻬ ﻭ ﻮ ﻯ ﻰ ﻱ"
#define UNICODE_HALFWIDTH_AND_FULLWIDTH_FORMS "！ ＂ ＃ ＄ ％ ＆ ＇ （ ） ＊ ＋ ， － ． ／ ０ １ ２ ３ ４ ５ ６ ７ ８ ９ ： ； ＜ ＝ ＞ ？ ＠ Ａ Ｂ Ｃ Ｄ Ｅ Ｆ Ｇ Ｈ Ｉ Ｊ Ｋ Ｌ Ｍ Ｎ Ｏ Ｐ Ｑ Ｒ Ｓ Ｔ Ｕ Ｖ Ｗ Ｘ Ｙ Ｚ ［ ＼ ］ ＾ ＿ ｀ ａ ｂ ｃ ｄ ｅ ｆ ｇ ｈ ｉ ｊ ｋ ｌ ｍ ｎ ｏ ｐ ｑ ｒ ｓ ｔ ｕ ｖ ｗ ｘ ｙ ｚ ｛ ｜ ｝ ～ ｡ ｢ ｣ ､ ･ ｦ ｧ ｨ ｩ ｪ ｫ ｬ ｭ ｮ ｯ ｰ ｱ ｲ ｳ ｴ ｵ ｶ ｷ ｸ ｹ ｺ ｻ ｼ ｽ ｾ ｿ ﾀ ﾁ ﾂ"
#define UNICODE_MUSICAL "턀 턁 턂 턃 턄 턅 턆 턇 턈 턉 턊 턋 턌 턍 턎 턏 턐 턑 턒 턓 턔 턕 턖 턗 턘 턙 턚 턛 턜 턝 턞 턟 턠 턡 턢 턣 턤 턥 턦 턪 턫 턬 턭 턮 턯 터 턱 턲 턳 턴 턵 턶 턷 털 턹 턺 턻 턼 턽 턾 턿 텀 텁 텂 텃 텄 텅 텆 텇 텈 텉 텊 텋 테 텍 텎 텏 텐 텑 텒 텓 텔 텕 텖 텗 텘 텙 텚 텛 템 텝 텞 텟 텠 텡 텢 텣 텤 텥 텦 텧 텨 텩 텪 텫 텬 텭 텮 텯 텰 텱 텲 텳 텴 텵 텶 텷 텸 텹 텺 텻 텼 텽 텾 텿 톀 톁 톂 "
#define UNICODE_SPECIALS "￹ ￺ ￻ ￼ �"
#define UNICODE_OLD_ITALIC "̀ ́ ̂ ̃ ̄ ̅ ̆ ̇ ̈ ̉ ̊ ̋ ̌ ̍ ̎ ̏ ̐ ̑ ̒ ̓ ̔ ̕ ̖ ̗ ̘ ̙ ̚ ̛ ̜ ̝ ̞ ̠ ̡ ̢ ̣"
#define UNICODE_GOTHIC "̰ ̱ ̲ ̳ ̴ ̵ ̶ ̷ ̸ ̹ ̺ ̻ ̼ ̽ ̾ ̿ ̀ ́ ͂ ̓ ̈́ ͅ ͆ ͇ ͈ ͉ ͊"
#define UNICODE_DESERET "Ѐ Ё Ђ Ѓ Є Ѕ І Ї Ј Љ Њ Ћ Ќ Ѝ Ў Џ А Б В Г Д Е Ж З И Й К Л М Н О П Р С Т У Ф Х Ш Щ Ъ Ы Ь Э Ю Я а б в г д е ж з и й к л м н о п р с т у ф х ц ч ш щ ъ ы ь э"
#define UNICODE_BRAILLE "⠀ ⠁ ⠂ ⠃ ⠄ ⠅ ⠆ ⠇ ⠈ ⠉ ⠊ ⠋ ⠌ ⠍ ⠎ ⠏ ⠐ ⠑ ⠒ ⠓ ⠔ ⠕ ⠖ ⠗ ⠘ ⠙ ⠚ ⠛ ⠜ ⠝ ⠞ ⠟ ⠠ ⠡ ⠢ ⠣ ⠤ ⠥ ⠦ ⠧ ⠨ ⠩ ⠪ ⠫ ⠬ ⠭ ⠮ ⠯ ⠰ ⠱ ⠲ ⠳ ⠴ ⠵ ⠶ ⠷ ⠸ ⠹ ⠺ ⠻ ⠼ ⠽ ⠾ ⠿ ⡀ ⡁ ⡂ ⡃ ⡄ ⡅ ⡆ ⡇ ⡈ ⡉ ⡊ ⡋ ⡌ ⡍ ⡎ ⡏ ⡐ ⡑ ⡒ ⡓ ⡔ ⡕ ⡖ ⡗ ⡘ ⡙ ⡚ ⡛ ⡜ ⡝ ⡞ ⡟ ⡠ ⡡ ⡢ ⡣ ⡤ ⡥ ⡦ ⡧ ⡨ ⡩ ⡪ ⡫ ⡬ ⡭ ⡮ ⡯ ⡰ ⡱ ⡲ ⡳ ⡴ ⡵ ⡶ ⡷ ⡸ ⡹ ⡺ ⡻ ⡼ ⡽ ⡾ ⡿"
	
	setlocale(LC_ALL, ".utf8");
	printf("-------- Expected --------\n");
	printf("%ls\n", L"a b c d Д Е Ж З И Й К Л У Ф Х ≤ ≥ ♪ ♫ ╞ ╟ ╠ ╡ ╢ ╣");
	printf("%s\n", "\xD0\x94");
	
	//// UTF-8 ////
	printf("-------- UTF-8  --------\n");
	str8 test8 = str8_lit(u8"Ё Ђ Ѓ Є Ѕ І Ї Ј Љ Њ Ћ Ќ Ў Џ А Б В Г Д Е Ж З И Й К Л М Н О П Р С Т У Ф Х Ц Ч Ш Щ Ъ Ы Ь Э Ю Я а");
	printf("%s\n", test8.str);
	
	//// UTF-16 ////
	printf("-------- UTF-16 --------\n");
	str16 test16 = str16_lit(u"Ё Ђ Ѓ Є Ѕ І Ї Ј Љ Њ Ћ Ќ Ў Џ А Б В Г Д Е Ж З И Й К Л М Н О П Р С Т У Ф Х Ц Ч Ш Щ Ъ Ы Ь Э Ю Я а");
	printf("%s\n", str8_from_str16(test16).str);
	
	//// UTF-32 ////
	printf("-------- UTF-32 --------\n");
	str32 test32 = str32_lit(U"Ё Ђ Ѓ Є Ѕ І Ї Ј Љ Њ Ћ Ќ Ў Џ А Б В Г Д Е Ж З И Й К Л М Н О П Р С Т У Ф Х Ц Ч Ш Щ Ъ Ы Ь Э Ю Я а");
	printf("%s\n", str8_from_str32(test32).str);
	
	printf("[DESHI-TEST] TODO:   utils/unicode\n");
}

#include "utils.h"
local void TEST_deshi_utils_utils(){
	printf("[DESHI-TEST] TODO:   utils/utils\n");
}

local void TEST_deshi_utils(){
	TEST_deshi_utils_array();
	TEST_deshi_utils_array_utils();
	TEST_deshi_utils_carray();
	TEST_deshi_utils_color();
	TEST_deshi_utils_cstring();
	TEST_deshi_utils_hash();
	TEST_deshi_utils_map();
	TEST_deshi_utils_optional();
	TEST_deshi_utils_ring_array();
	TEST_deshi_utils_string();
	TEST_deshi_utils_string_utils();
	TEST_deshi_utils_pair();
	TEST_deshi_utils_unicode();
	TEST_deshi_utils_utils();
}