#pragma once
#include <string>
#include <vector>
#include <boost/optional.hpp>

template<class T>
class ContainerManager {
public:
	//std::vector<std::pair<std::optional<T>, int>> container;

	std::vector<boost::optional<T>> container;
	std::vector<int> empties;

	int real_size = 0;

	ContainerManager() {}

	//T t& operator [](int i) { return container[i]; }
	//void operator = (ContainerManager<T> c) { this->copy(c); }

	int add_to(T t) {
		if (empties.size() == 0) {
			container.push_back(t);
			real_size++;
			return container.size() - 1;
		}
		else {
			container[empties[0]] = t;
			int index = empties[0];
			empties.erase(empties.begin());
			real_size++;
			return index;
		}
	}

	void add_to_index(T t, int index) {
		//ASSERT(allocate_space(index), "Container was unable to allocate space at specified index");
		if(!container[index]){ real_size++; }
		container[index] = t;
		
	}

	//idk why i have this?
	void set_index(T t, int index) {
		if (!container[index]) { real_size++; }
		container[index] = t;

	}

	void remove_from(int index) {
		//ASSERT(container[index], "Container at index " + std::to_string(index) + " is already empty.");
		//ASSERT(index <= container.size() - 1, "Trying to access container at an index that doesn't exist.");

		//if (index == container.size() - 1) {
		//	container.pop_back();
		//}
		//else {
		real_size--;
		container[index].first.reset();
		//empties.push_back(index);
	//}
	}

	bool allocate_space(int index) {
		//ASSERT(index >= 0, "Attempted to pass negative index.");
		//ASSERT(index >= container.size(), "Attempted to allocate space at an index larger than container size.");
		if (index < container.size()) {
			//space already exists, there is no check to actually see if
			//it's already used by something else yet though
			return true;
		}
		else {
			boost::optional<T> o;
			for (int i = container.size(); i < index; i++) {
				container.push_back(o);
			}

			return true;
		}
		return false;
	}

	int size() {
		return container.size();
	}

	void copy(ContainerManager<T> cm) {
		container = cm.container;
		empties = cm.empties;
	}

	void empty() {
		//this probably isn't what I should do so fix this if it isn't
		empties.clear();
		for (int i = 0; i < container.size(); i++) {
			container[i].first.reset();
			empties.push_back(i);
		}
		real_size = 0;
	}

	void clear() {
		container.clear();
		real_size = 0;
	}

	std::string str() {
		std::string s = "";
		int index = 1;
		for (auto t : container) {
			s += t.value()->str + " " + std::to_string(t.value()->index) + " " + std::to_string(index) + "\n";
			index++;
		}
		return s;
	}
};