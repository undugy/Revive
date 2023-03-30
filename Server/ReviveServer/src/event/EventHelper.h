#pragma once
class timer_event;
class EventHelper
{
public:
	EventHelper(){}
	~EventHelper(){}



	void ProcessTimer();
	void ProcessEvent(const timer_event& ev);
};

