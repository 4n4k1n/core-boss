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
		// Check if it's money or resources - miners should mine them, others move to them
		if (next_pos_obj->type == OBJ_MONEY)
		{
			core_action_move(unit, next_pos);
		}
		else if (next_pos_obj->type == OBJ_RESOURCE)
		{
			if (unit->s_unit.unit_type == UNIT_MINER)
			{
				core_action_attack(unit, next_pos); // Miners mine resources by attacking them
			}
			else
			{
				core_action_move(unit, next_pos); // Other units just move to resources
			}
		}
		// Check if it's an enemy unit or core - attack these
		else if (next_pos_obj->type == OBJ_UNIT && next_pos_obj->s_unit.team_id != game.my_team_id)
		{
			core_action_attack(unit, next_pos);
		}
		else if (next_pos_obj->type == OBJ_CORE && next_pos_obj->s_core.team_id != game.my_team_id)
		{
			core_action_attack(unit, next_pos);
		}
		// Try to walk around walls - try alternative directions
		else if (next_pos_obj->type == OBJ_WALL)
		{
			// Try moving in the other primary direction first
			t_pos alt_pos = { unit->pos.x, unit->pos.y };
			if (abs(dx) > abs(dy))
			{
				// We were moving horizontally, try vertical
				int step = (dy > 0) ? 1 : (dy < 0) ? -1 : (rand() % 2) ? 1 : -1;
				alt_pos.y += step;
			}
			else
			{
				// We were moving vertically, try horizontal
				int step = (dx > 0) ? 1 : (dx < 0) ? -1 : (rand() % 2) ? 1 : -1;
				alt_pos.x += step;
			}
			
			t_obj *alt_obj = core_get_obj_from_pos(alt_pos);
			if (!alt_obj)
			{
				core_action_move(unit, alt_pos);
			}
			else
			{
				// If can't walk around, attack the wall
				core_action_attack(unit, next_pos);
			}
		}
		// Don't attack our own units/core - just move if possible
		else
		{
			core_action_move(unit, next_pos);
		}
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

	// Check miner limit: resource_amount * 0.4 > own_workers + opponent_workers
	// int resource_count = ft_count_resources();
	int own_miners = ft_count_miners_own();
	// int opponent_miners = ft_count_miners_opponent();
	double max_miners = 3;
	
	// Spawn miners if we haven't hit the limit, otherwise spawn warriors
	if (core_own->s_core.balance >= 100 && (own_miners < max_miners))
	{
		core_action_createUnit(UNIT_MINER);
	}
	else if (core_own->s_core.balance >= 150)  // Warrior cost is 150
	{
		core_action_createUnit(UNIT_WARRIOR);
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
			t_obj *unit = units[i];
			if (unit->state != STATE_ALIVE)
				continue;
			
			if (unit->s_unit.unit_type == UNIT_MINER)
			{
				t_obj *assigned_resource = NULL;
				double best_distance = -1;
				int best_resource_idx = -1;
				
				// Find nearest unassigned resource
				for (int j = 0; j < resource_count; j++)
				{
					if (resource_assigned[j])
						continue;
						
					double distance = ft_calculate_distance(unit->pos, all_resources[j]->pos);
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
					
					// Check if miner should return to core
					// Return when: 1) holding balance >= 300, or 2) no resources left
					bool should_return = (unit->s_unit.balance >= 300) || 
					                    (resource_count == 0);
					
					if (unit->s_unit.balance <= 0 || !should_return)
					{
						// Go to assigned resource to mine
						move_and_attack(unit, assigned_resource->pos);
					}
					else
					{
						// Return to core and transfer money
						move_and_attack(unit, core_own->pos);
						core_action_transferMoney(unit, core_own->pos, unit->s_unit.balance);
					}
				}
			}
			else if (unit->s_unit.unit_type == UNIT_WARRIOR)
			{
				// Warriors attack nearest enemy unit, or enemy core if no units
				t_obj *closest_opponent = ft_get_units_opponent_nearest(unit->pos);
				if (closest_opponent)
					move_and_attack(unit, closest_opponent->pos);
				else
				{
					t_obj *enemy_core = ft_get_core_opponent();
					if (enemy_core)
						move_and_attack(unit, enemy_core->pos);
				}
			}
		}
		
		free(resource_assigned);
	}
	
	if (all_resources) free(all_resources);
	if (units) free(units);
}