#ifndef INC_MODULE_MATCH_H
#define INC_MODULE_MATCH_H

#pragma once

#include <vector>
#include <set>
#include <utility>
#include <opencv2\core\types.hpp>

#pragma push_macro("_STD")
#undef _STD
#define _STD std::

#define _CV cv::


class PeopleModule
{
public:
	PeopleModule();
	PeopleModule(_CV Rect2f &rect);
	PeopleModule(_CV Rect2f &rect, int life);
	void decrease_life();
	int life()const;
	void reborn();
	void modify_rect(_CV Rect2f &rect);
	const _CV Rect2f get_rect()const;
	void set_id(size_t id);
	size_t id()const;
private:
	int _life;
	const static int LifeTime;
	_CV Rect2f rect;
	size_t _id;
};


class MatchPeopleModule
{
public:
	using VecRect = _STD vector<_CV Rect2f>;
	using Index = size_t;
	using TargetIndex = size_t;
	using ModuleIndex = size_t;
	using MyPair = _STD pair <ModuleIndex, TargetIndex>;
	MatchPeopleModule();
	void update_module(VecRect &rects);
	_STD vector<PeopleModule> get_modules()const;
private:
	void setup_match_matrix(VecRect &rects);
	MyPair find_max_pos(_STD set<TargetIndex> &target_id_set, _STD set<ModuleIndex> &module_id_set);
private:
	_STD vector<PeopleModule> modules;
	size_t candidate_pos;
	_STD vector<_STD vector<float>> match_matrix;
	const static int CandidateLife;
	size_t id_now;
};



#undef _STD
#pragma pop_macro("_STD")

#endif // !INC_MODULE_MATCH_H
