#include "module_match.h"
#include <cmath>
#include <algorithm>

const int PeopleModule::LifeTime = 40;
const int MatchPeopleModule::CandidateLife = 5;

PeopleModule::PeopleModule()
	:_life(-1), rect(), _id(0)
{

}
PeopleModule::PeopleModule(_CV Rect2f &rect)
	:_life(LifeTime), rect(rect), _id(0)
{

}
PeopleModule::PeopleModule(_CV Rect2f &rect, int life)
	:_life(life), rect(rect), _id(0)
{

}
inline void PeopleModule::decrease_life()
{
	_life--;
}
inline void PeopleModule::reborn()
{
	_life = LifeTime;
}
inline int PeopleModule::life() const
{
	return _life;
}
inline void PeopleModule::modify_rect(_CV Rect2f &rect)
{
	this->rect = rect;
}
inline const _CV Rect2f PeopleModule::get_rect()const
{
	return rect;
}
inline void PeopleModule::set_id(size_t id)
{
	_id = id;
}
size_t PeopleModule::id()const
{
	return _id;
}

using Index = MatchPeopleModule::Index;
using TargetIndex = MatchPeopleModule::TargetIndex;
using ModuleIndex = MatchPeopleModule::ModuleIndex;
using MyPair = MatchPeopleModule::MyPair;

MatchPeopleModule::MatchPeopleModule()
	:modules(), candidate_pos(0), id_now(1)
{

}
void initial_id_set(_STD set<Index> &set, Index end, Index begin = 0)	// "end" will not be added to the set
{
	for (Index id = begin; id < end; id++)
	{
		set.insert(id);
	}
}
MyPair MatchPeopleModule::find_max_pos(_STD set<TargetIndex> &target_id_set, _STD set<ModuleIndex> &module_id_set)
{
	float max_val = match_matrix[0][0];
	TargetIndex t_id_max = *target_id_set.begin();
	ModuleIndex m_id_max = *module_id_set.begin();

	for (auto target_iter = target_id_set.begin(); target_iter != target_id_set.end(); ++target_iter)
	{
		for (auto module_iter = module_id_set.begin(); module_iter != module_id_set.end(); ++module_iter)
		{
			if (match_matrix[*module_iter][*target_iter] > max_val)
			{
				max_val = match_matrix[*module_iter][*target_iter];
				t_id_max = *target_iter;
				m_id_max = *module_iter;
			}
		}
	}
	return MyPair(m_id_max, t_id_max);
}
void MatchPeopleModule::update_module(VecRect &rects)
{
	if (rects.size() == 0)
	{
		// decrease all modules life
		int i = 0;
		for (auto iter = modules.begin(); iter != modules.end(); ++i)
		{
			iter->decrease_life();
			if (iter->life() <= 0)
			{
				iter = modules.erase(iter);
			}
			else
			{
				++iter;
			}
			if (i == candidate_pos)
			{
				candidate_pos = iter - modules.begin();
			}
		}
		// done
		return;
	}
	if (modules.size() != 0)
	{
		setup_match_matrix(rects);
		_STD set<ModuleIndex> module_id;
		_STD set<TargetIndex> target_id;
		initial_id_set(module_id, modules.size());
		initial_id_set(target_id, rects.size());
		// match modules and targets
		int i = 0;
		while (module_id.size() > 0 && target_id.size() > 0 && 
			i++ < _STD min(match_matrix.size(), match_matrix[0].size()))
		{
			MyPair p = find_max_pos(target_id, module_id);
			if (match_matrix[p.first][p.second] < 0.01)
				continue;
			module_id.erase(p.first);
			target_id.erase(p.second);
			modules.at(p.first).reborn();
			modules.at(p.first).modify_rect(rects.at(p.second));
		}
		// update old module
		for (auto iter = module_id.begin(); iter != module_id.end(); ++iter)
		{
			modules.at(*iter).decrease_life();
		}
		// seprate candidates
		_STD vector<PeopleModule> candidates;
		candidates.assign(modules.begin() + candidate_pos, modules.end());
		// clear all old modules
		size_t vaild_id = 0;
		modules.resize(candidate_pos);
		for (auto iter = modules.begin(); iter != modules.end(); )
		{
			if (iter->life() <= 0)
			{
				iter = modules.erase(iter);
			}
			else
			{
				++iter;
				++vaild_id;
			}
		}
		modules.resize(vaild_id);
		// remove bad candidates
		for (auto iter = candidates.begin(); iter != candidates.end();)
		{
			if (iter->life() <= 0)
			{
				iter = candidates.erase(iter);
			}
			else
			{
				iter->set_id(id_now++);
				iter++;
				
			}
		}
		// upgrade candidates
		modules.insert(modules.end(), candidates.begin(), candidates.end());
		candidate_pos = modules.end() - modules.begin();
		// new cadidates
		for (auto iter = target_id.begin(); iter != target_id.end(); iter++)
		{
			modules.push_back(PeopleModule(rects.at(*iter), CandidateLife));	// set candidates life be CandidateLife
		}
	}
	else
	{
		// add them to the modules
		modules.reserve(rects.size());
		for (auto iter = rects.begin(); iter != rects.end(); ++iter)
		{
			modules.push_back(PeopleModule(*iter, CandidateLife));
		}
		candidate_pos = 0;
	}
}
float IOU(const _CV Rect2f& box1, const _CV Rect2f& box2)
{
	if (box1.x > box2.x + box2.width)
	{
		return 0.0;
	}
	if (box1.y > box2.y + box2.height)
	{
		return 0.0;
	}
	if (box1.x + box1.width < box2.x)
	{
		return 0.0;
	}
	if (box1.y + box1.height < box2.y)
	{
		return 0.0;
	}
	float colInt = _STD min(box1.x + box1.width, box2.x + box2.width) - _STD max(box1.x, box2.x);
	float rowInt = _STD min(box1.y + box1.height, box2.y + box2.height) - _STD max(box1.y, box2.y);
	float intersection = colInt * rowInt;
	float area1 = box1.width*box1.height;
	float area2 = box2.width*box2.height;
	return intersection / (area1 + area2 - intersection);
}
void MatchPeopleModule::setup_match_matrix(VecRect &rects)
{
	match_matrix.clear();
	match_matrix.resize(modules.size());
	// create matrix
	for (auto iter_module = match_matrix.begin(); iter_module != match_matrix.end(); ++iter_module)
	{
		iter_module->resize(rects.size());
		for (auto iter_obj = iter_module->begin(); iter_obj != iter_module->end(); ++iter_obj)
		{
			*iter_obj = IOU(rects.at(iter_obj - iter_module->begin()), 
				modules.at(iter_module - match_matrix.begin()).get_rect());
		}
	}
}
_STD vector<PeopleModule> MatchPeopleModule::get_modules()const
{
	_STD vector<PeopleModule> res;
	res.assign(modules.begin(), modules.begin() + candidate_pos);
	return res;
}
