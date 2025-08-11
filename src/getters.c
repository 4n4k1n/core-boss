#include "bot.h"

static bool is_core(const t_obj *obj)
{
	return (obj->type == OBJ_CORE && obj->state == STATE_ALIVE);
}
static bool is_core_own(const t_obj *obj)
{
	return (is_core(obj) && obj->s_core.team_id == game.my_team_id);
}
static bool is_core_opponent(const t_obj *obj)
{
	return (is_core(obj) && obj->s_core.team_id != game.my_team_id);
}

static bool is_resource(const t_obj *obj)
{
	return (obj->type == OBJ_RESOURCE && obj->state == STATE_ALIVE);
}
static bool is_money(const t_obj *obj)
{
	return (obj->type == OBJ_MONEY && obj->state == STATE_ALIVE);
}
static bool is_resource_money(const t_obj *obj)
{
	return (is_resource(obj) || is_money(obj));
}

static bool is_unit(const t_obj *obj)
{
	return (obj->type == OBJ_UNIT && obj->state == STATE_ALIVE);
}
static bool is_unit_own(const t_obj *obj)
{
	return (is_unit(obj) && obj->s_unit.team_id == game.my_team_id);
}
static bool is_unit_opponent(const t_obj *obj)
{
	return (is_unit(obj) && obj->s_unit.team_id != game.my_team_id);
}

// -

t_obj *ft_get_core_own(void)
{
	t_pos pos = {0, 0}; // Position doesn't matter for this search
	return core_get_obj_customCondition_nearest(pos, is_core_own);
}
t_obj *ft_get_core_opponent(void)
{
	t_pos pos = {0, 0}; // Position doesn't matter for this search
	return core_get_obj_customCondition_nearest(pos, is_core_opponent);
}

t_obj *ft_get_resource_nearest(t_pos pos)
{
	return core_get_obj_customCondition_nearest(pos, is_resource);
}
t_obj *ft_get_money_nearest(t_pos pos)
{
	return core_get_obj_customCondition_nearest(pos, is_money);
}
t_obj *ft_get_resource_money_nearest(t_pos pos)
{
	return core_get_obj_customCondition_nearest(pos, is_resource_money);
}

t_obj **ft_get_units_own(void)
{
	return core_get_objs_customCondition(is_unit_own);
}
t_obj **ft_get_units_opponent(void)
{
	return core_get_objs_customCondition(is_unit_opponent);
}
t_obj *ft_get_units_opponent_nearest(t_pos pos)
{
	return core_get_obj_customCondition_nearest(pos, is_unit_opponent);
}

int ft_count_resources(void)
{
	t_obj **resources = core_get_objs_customCondition(is_resource_money);
	int count = 0;
	if (resources)
	{
		while (resources[count])
			count++;
		free(resources);
	}
	return count;
}

int ft_count_miners_own(void)
{
	t_obj **units = ft_get_units_own();
	int count = 0;
	if (units)
	{
		for (int i = 0; units[i]; i++)
		{
			if (units[i]->s_unit.unit_type == UNIT_MINER)
				count++;
		}
		free(units);
	}
	return count;
}

int ft_count_miners_opponent(void)
{
	t_obj **units = ft_get_units_opponent();
	int count = 0;
	if (units)
	{
		for (int i = 0; units[i]; i++)
		{
			if (units[i]->s_unit.unit_type == UNIT_MINER)
				count++;
		}
		free(units);
	}
	return count;
}

t_obj **ft_get_all_resources(void)
{
	return core_get_objs_customCondition(is_resource_money);
}

double ft_calculate_distance(t_pos pos1, t_pos pos2)
{
	int dx = pos1.x - pos2.x;
	int dy = pos1.y - pos2.y;
	return (dx * dx + dy * dy);
}

t_obj *ft_find_nearest_unassigned_resource(t_pos miner_pos, t_obj **all_miners)
{
	t_obj **all_resources = ft_get_all_resources();
	if (!all_resources)
		return NULL;

	t_obj *best_resource = NULL;
	double best_distance = -1;

	for (int i = 0; all_resources[i]; i++)
	{
		t_obj *resource = all_resources[i];
		bool is_assigned = false;

		// Check if this resource is already assigned to another miner
		for (int j = 0; all_miners && all_miners[j]; j++)
		{
			t_obj *other_miner = all_miners[j];
			if (other_miner->s_unit.unit_type != UNIT_MINER)
				continue;

			// Check if other miner is closer to this resource than to any other
			t_obj *other_target = ft_get_resource_money_nearest(other_miner->pos);
			if (other_target && other_target->id == resource->id && 
				other_miner->pos.x != miner_pos.x && other_miner->pos.y != miner_pos.y)
			{
				is_assigned = true;
				break;
			}
		}

		if (!is_assigned)
		{
			double distance = ft_calculate_distance(miner_pos, resource->pos);
			if (best_distance < 0 || distance < best_distance)
			{
				best_distance = distance;
				best_resource = resource;
			}
		}
	}

	free(all_resources);
	return best_resource;
}
