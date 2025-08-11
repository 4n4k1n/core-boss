#include "bot.h"

#include <time.h>
#include <stdio.h>
#include <stdlib.h>

void ft_on_tick(unsigned long tick);

int	main(int argc, char **argv)
{
	return core_startGame("ur_mom", argc, argv, ft_on_tick, false);
}

void move_and_attack(t_obj *unit, t_pos target_pos)
{
	int dx = target_pos.x - unit->pos.x;
	int dy = target_pos.y - unit->pos.y;

	t_pos next_pos = { unit->pos.x, unit->pos.y };
	if (abs(dx) > abs(dy))
	{
		int step = (dx > 0) ? 1 : -1;
		next_pos.x += step;
	}
	else
	{
		int step = (dy > 0) ? 1 : -1;
		next_pos.y += step;
	}

	t_obj *next_pos_obj = core_get_obj_from_pos(next_pos);
	if (next_pos_obj)
	{
		if (next_pos_obj->type == OBJ_MONEY)
			core_action_move(unit, next_pos);
		else
			core_action_attack(unit, next_pos);
	}
	else
	{
		core_action_move(unit, next_pos);
	}
}

void ft_on_tick(unsigned long tick)
{
	(void)tick;
	
	t_obj *core_own = ft_get_core_own();
	if (!core_own)
		return;

	// Check miner limit: resource_amount * 0.75 > own_workers + opponent_workers
	int resource_count = ft_count_resources();
	int own_miners = ft_count_miners_own();
	int opponent_miners = ft_count_miners_opponent();
	double max_miners = resource_count * 0.75;
	
	// Only spawn miners if we have enough money and haven't hit the limit
	if (core_own->s_core.balance >= 100 && (own_miners + opponent_miners) < max_miners)
	{
		core_action_createUnit(UNIT_MINER);
	}

	// Get all resources and assign miners 1:1
	t_obj **all_resources = ft_get_all_resources();
	t_obj **units = ft_get_units_own();
	
	// Create assignment array - each miner gets assigned to exactly one resource
	if (all_resources && units)
	{
		// Count miners
		int miner_count = 0;
		for (int i = 0; units[i]; i++)
		{
			if (units[i]->state == STATE_ALIVE && units[i]->s_unit.unit_type == UNIT_MINER)
				miner_count++;
		}
		
		// Count resources
		int resource_count = 0;
		for (int i = 0; all_resources[i]; i++)
			resource_count++;
		
		// Assign each miner to the nearest available resource
		bool *resource_assigned = calloc(resource_count, sizeof(bool));
		
		for (int i = 0; units && units[i]; i++)
		{
			t_obj *miner = units[i];
			if (miner->state != STATE_ALIVE || miner->s_unit.unit_type != UNIT_MINER)
				continue;
			
			t_obj *assigned_resource = NULL;
			double best_distance = -1;
			int best_resource_idx = -1;
			
			// Find nearest unassigned resource
			for (int j = 0; j < resource_count; j++)
			{
				if (resource_assigned[j])
					continue;
					
				double distance = ft_calculate_distance(miner->pos, all_resources[j]->pos);
				if (best_distance < 0 || distance < best_distance)
				{
					best_distance = distance;
					assigned_resource = all_resources[j];
					best_resource_idx = j;
				}
			}
			
			if (assigned_resource)
			{
				resource_assigned[best_resource_idx] = true;
				
				// If miner has no money, go to assigned resource
				if (miner->s_unit.balance <= 0)
				{
					move_and_attack(miner, assigned_resource->pos);
				}
				// If miner has money, return to core and transfer it
				else
				{
					move_and_attack(miner, core_own->pos);
					core_action_transferMoney(miner, core_own->pos, miner->s_unit.balance);
				}
			}
		}
		
		free(resource_assigned);
	}
	
	if (all_resources) free(all_resources);
	if (units) free(units);
}