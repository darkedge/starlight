#pragma once
template<typename Object, typename Event>
class Observer {
public:
	Observer();
	virtual ~Observer();
	virtual void onNotify(Object* object, Event event) = 0;
};

