#pragma once
#include "../objective.h"
#include "../config/config.h"
#include "../numericTypes/types.h"
#include "../numericTypes/index.h"

struct DeserializationMemo;
class Area;

class UnmountObjective final : public Objective
{
	Point3D m_location;
public:
	UnmountObjective(const Point3D& location) : Objective(Config::goToPriority), m_location(location) { }
	UnmountObjective(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const override;
	void execute(Area& area, const ActorIndex& actor) override;
	void cancel(Area&, const ActorIndex&) override { }
	void delay(Area&, const ActorIndex&) override { }
	void reset(Area&, const ActorIndex&) override { }
	std::string name() const override { return "unmount"; }
};