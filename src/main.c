#include "bot.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

// Constants
#define STARTING_MONEY 200
#define MINER_RETURN_THRESHOLD 300
#define CARRIER_MAX_BALANCE 100

// Function declarations
void ft_on_tick(unsigned long tick);
void move_and_attack(t_obj *unit, t_pos target_pos);
void move_and_attack_avoid_money(t_obj *unit, t_pos target_pos, bool avoid_money);
t_pos try_alternative_move(t_pos unit_pos, int dx, int dy);
bool should_attack_object(t_obj *unit, t_obj *target);
void handle_unit_spawning(t_obj *core_own);
void control_all_units(t_obj *core_own);
void control_miner(t_obj *miner, t_obj *core_own, t_obj **all_resources, bool *resource_assigned, int resource_count);
void control_carrier(t_obj *carrier, t_obj *core_own);
void control_warrior(t_obj *warrior);
t_obj *find_assigned_resource_for_miner(t_obj *miner, t_obj *core_own, t_obj **all_resources, bool *resource_assigned, int resource_count, bool has_carrier);
t_obj *find_target_miner_for_carrier(t_obj *carrier);
t_obj *find_low_hp_resource(t_obj *carrier);
int get_carrier_index(t_obj *carrier);
void count_unit_types(int *miners, int *carriers, int *warriors);

int main(int argc, char **argv)
{
	return core_startGame("ur_mom", argc, argv, ft_on_tick, false);
}

void ft_on_tick(unsigned long tick)
{
	(void)tick;
	
	t_obj *core_own = ft_get_core_own();
	if (!core_own)
		return;

	handle_unit_spawning(core_own);
	control_all_units(core_own);
}

void handle_unit_spawning(t_obj *core_own)
{
	int own_miners, own_carriers, own_warriors;
	count_unit_types(&own_miners, &own_carriers, &own_warriors);

	// New strategy: Carrier first -> Worker -> then normal sequence
	if (own_carriers == 0 && core_own->s_core.balance >= 200)
	{
		core_action_createUnit(UNIT_CARRIER);
	}
	// After first carrier + miner, continue normal spawning
	else if (own_miners < 2 && core_own->s_core.balance >= 100)
	{
		core_action_createUnit(UNIT_MINER);
	}
	else if (own_carriers < 2 && core_own->s_core.balance >= 200)
	{
		core_action_createUnit(UNIT_CARRIER);
	}
	// else if (own_miners < 3 && own_carriers >= 2 && core_own->s_core.balance >= 100)
	// {
	// 	core_action_createUnit(UNIT_MINER);
	// }
	else if (core_own->s_core.balance >= 150)
	{
		core_action_createUnit(UNIT_WARRIOR);
	}
}

void control_all_units(t_obj *core_own)
{
	t_obj **all_resources = ft_get_all_resources();
	t_obj **units = ft_get_units_own();
	
	if (!all_resources || !units)
		return (free(all_resources), free(units));

	// Count resources
	int resource_count = 0;
	for (int i = 0; all_resources[i]; i++)
		resource_count++;
	
	// Create resource assignment tracking
	bool *resource_assigned = calloc(resource_count, sizeof(bool));
	
	// Control each unit based on its type
	for (int i = 0; units[i]; i++)
	{
		t_obj *unit = units[i];
		if (unit->state != STATE_ALIVE)
			continue;
		
		switch (unit->s_unit.unit_type)
		{
			case UNIT_MINER:
				control_miner(unit, core_own, all_resources, resource_assigned, resource_count);
				break;
			case UNIT_CARRIER:
				control_carrier(unit, core_own);
				break;
			case UNIT_WARRIOR:
				control_warrior(unit);
				break;
		}
	}
	
	free(resource_assigned);
	free(all_resources);
	free(units);
}

void control_miner(t_obj *miner, t_obj *core_own, t_obj **all_resources, bool *resource_assigned, int resource_count)
{
	// Check if there are any carriers alive
	int own_miners, own_carriers, own_warriors;
	count_unit_types(&own_miners, &own_carriers, &own_warriors);
	bool has_carrier = (own_carriers > 0);
	
	// If miner has money but there's a carrier, immediately return to deposit
	if (has_carrier && miner->s_unit.balance > 0)
	{
		move_and_attack(miner, core_own->pos);
		if (miner->s_unit.move_cooldown == 0)
			core_action_transferMoney(miner, core_own->pos, miner->s_unit.balance);
		return;
	}
	
	t_obj *assigned_resource = find_assigned_resource_for_miner(miner, core_own, all_resources, resource_assigned, resource_count, has_carrier);
	
	if (assigned_resource)
	{
		// Mark resource as assigned
		for (int j = 0; j < resource_count; j++)
		{
			if (all_resources[j]->id == assigned_resource->id)
			{
				resource_assigned[j] = true;
				break;
			}
		}
		
		bool should_return = (miner->s_unit.balance >= MINER_RETURN_THRESHOLD) || (resource_count == 0);
		
		if (miner->s_unit.balance <= 0 || !should_return)
		{
			move_and_attack_avoid_money(miner, assigned_resource->pos, has_carrier);
		}
		else
		{
			move_and_attack(miner, core_own->pos);
			if (miner->s_unit.move_cooldown == 0)
				core_action_transferMoney(miner, core_own->pos, miner->s_unit.balance);
		}
	}
	else if (resource_count == 0)
	{
		// No resources left
		if (miner->s_unit.balance > 0)
		{
			// Deposit any remaining money
			move_and_attack(miner, core_own->pos);
			if (miner->s_unit.move_cooldown == 0)
				core_action_transferMoney(miner, core_own->pos, miner->s_unit.balance);
		}
		else if (!has_carrier)
		{
			// If no carrier exists, collect money from ground
			t_obj *nearest_money = ft_get_money_nearest(miner->pos);
			if (nearest_money)
			{
				move_and_attack(miner, nearest_money->pos);
			}
			else
			{
				// No money available, attack enemy
				t_obj *enemy_core = ft_get_core_opponent();
				if (enemy_core)
					move_and_attack(miner, enemy_core->pos);
			}
		}
		else
		{
			// Carrier exists, attack enemy instead of collecting money
			t_obj *enemy_core = ft_get_core_opponent();
			if (enemy_core)
				move_and_attack(miner, enemy_core->pos);
		}
	}
	else
	{
		// Resources exist but none assigned (all taken by other miners)
		// Attack enemies instead of waiting
		t_obj *enemy_core = ft_get_core_opponent();
		if (enemy_core)
			move_and_attack_avoid_money(miner, enemy_core->pos, has_carrier);
	}
}

void control_carrier(t_obj *carrier, t_obj *core_own)
{
	// Carriers only collect money, return to core when no money available
	t_obj *nearest_money = ft_get_money_nearest(carrier->pos);
	
	bool should_return = (carrier->s_unit.balance > 0);
	
	if (!should_return && nearest_money)
	{
		// Collect money from ground
		move_and_attack(carrier, nearest_money->pos);
	}
	else if (should_return)
	{
		// Return to core to deposit
		move_and_attack(carrier, core_own->pos);
		if (carrier->s_unit.move_cooldown == 0)
			core_action_transferMoney(carrier, core_own->pos, carrier->s_unit.balance);
	}
	else
	{
		// No money available, return to core
		move_and_attack(carrier, core_own->pos);
	}
}

void control_warrior(t_obj *warrior)
{
	t_obj *closest_opponent = ft_get_units_opponent_nearest(warrior->pos);
	if (closest_opponent)
	{
		move_and_attack(warrior, closest_opponent->pos);
	}
	else
	{
		t_obj *enemy_core = ft_get_core_opponent();
		if (enemy_core)
			move_and_attack(warrior, enemy_core->pos);
	}
}

t_obj *find_assigned_resource_for_miner(t_obj *miner, t_obj *core_own, t_obj **all_resources, bool *resource_assigned, int resource_count, bool has_carrier)
{
	t_obj *best_resource = NULL;
	double best_distance = -1;
	
	for (int j = 0; j < resource_count; j++)
	{
		if (resource_assigned[j])
			continue;
			
		// If no carrier, prioritize resources closest to core
		// If carrier exists, prioritize resources closest to miner
		double distance;
		if (!has_carrier)
			distance = ft_calculate_distance(core_own->pos, all_resources[j]->pos);
		else
			distance = ft_calculate_distance(miner->pos, all_resources[j]->pos);
		
		if (best_distance < 0 || distance < best_distance)
		{
			best_distance = distance;
			best_resource = all_resources[j];
		}
	}
	
	return best_resource;
}

t_obj *find_target_miner_for_carrier(t_obj *carrier)
{
	t_obj **all_units = ft_get_units_own();
	t_obj *target_miner = NULL;
	
	if (!all_units)
		return NULL;
	
	int carrier_index = get_carrier_index(carrier);
	int miner_count = 0;
	
	for (int k = 0; all_units[k]; k++)
	{
		if (all_units[k]->s_unit.unit_type == UNIT_MINER && all_units[k]->s_unit.balance > 0)
		{
			if (miner_count == carrier_index)
			{
				target_miner = all_units[k];
				break;
			}
			miner_count++;
		}
	}
	
	free(all_units);
	return target_miner;
}

// t_obj *find_low_hp_resource(t_obj *carrier)
// {
// 	t_obj **all_resources = ft_get_all_resources();
// 	t_obj *best_resource = NULL;
// 	double best_distance = -1;
	
// 	if (!all_resources)
// 		return NULL;
	
// 	for (int i = 0; all_resources[i]; i++)
// 	{
// 		t_obj *resource = all_resources[i];
		
// 		// Only look for resources with 1 HP
// 		if (resource->hp == 1)
// 		{
// 			double distance = ft_calculate_distance(carrier->pos, resource->pos);
// 			if (best_distance < 0 || distance < best_distance)
// 			{
// 				// printf("FOUND!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
// 				best_distance = distance;
// 				best_resource = resource;
// 			}
// 		}
// 	}
	
// 	free(all_resources);
// 	return best_resource;
// }

int get_carrier_index(t_obj *carrier)
{
	t_obj **all_units = ft_get_units_own();
	int carrier_index = 0;
	int current_carrier = 0;
	
	if (!all_units)
		return 0;
	
	for (int k = 0; all_units[k]; k++)
	{
		if (all_units[k]->s_unit.unit_type == UNIT_CARRIER)
		{
			if (all_units[k]->id == carrier->id)
			{
				carrier_index = current_carrier;
				break;
			}
			current_carrier++;
		}
	}
	
	free(all_units);
	return carrier_index;
}

void count_unit_types(int *miners, int *carriers, int *warriors)
{
	*miners = ft_count_miners_own();
	*carriers = 0;
	*warriors = 0;
	
	t_obj **units = ft_get_units_own();
	if (!units)
		return;
	
	for (int i = 0; units[i]; i++)
	{
		if (units[i]->state == STATE_ALIVE)
		{
			if (units[i]->s_unit.unit_type == UNIT_CARRIER)
				(*carriers)++;
			else if (units[i]->s_unit.unit_type == UNIT_WARRIOR)
				(*warriors)++;
		}
	}
	
	free(units);
}

void move_and_attack(t_obj *unit, t_pos target_pos)
{
	// Check if unit is on cooldown - skip if it can't act
	if (unit->s_unit.move_cooldown > 0)
		return;
		
	int dx = target_pos.x - unit->pos.x;
	int dy = target_pos.y - unit->pos.y;

	t_pos next_pos = { unit->pos.x, unit->pos.y };
	if (abs(dx) > abs(dy))
	{
		next_pos.x += (dx > 0) ? 1 : -1;
	}
	else
	{
		next_pos.y += (dy > 0) ? 1 : -1;
	}

	t_obj *next_pos_obj = core_get_obj_from_pos(next_pos);
	if (next_pos_obj)
	{
		if (should_attack_object(unit, next_pos_obj))
		{
			core_action_attack(unit, next_pos);
		}
		else if (next_pos_obj->type == OBJ_WALL || 
		         (next_pos_obj->type == OBJ_UNIT && next_pos_obj->s_unit.team_id == game.my_team_id) ||
		         (next_pos_obj->type == OBJ_RESOURCE && unit->s_unit.unit_type != UNIT_MINER))
		{
			// Try alternative path for walls, friendly units, and resources (for non-miners)
			t_pos alt_pos = try_alternative_move(unit->pos, dx, dy);
			
			// If alternative found a valid move, use it
			if (alt_pos.x != unit->pos.x || alt_pos.y != unit->pos.y)
			{
				core_action_move(unit, alt_pos);
			}
			else if (next_pos_obj->type == OBJ_WALL)
			{
				core_action_attack(unit, next_pos); // Attack wall if can't go around
			}
			// Stay put if can't find alternative and can't attack
		}
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

void move_and_attack_avoid_money(t_obj *unit, t_pos target_pos, bool avoid_money)
{
	// Check if unit is on cooldown - skip if it can't act
	if (unit->s_unit.move_cooldown > 0)
		return;
		
	int dx = target_pos.x - unit->pos.x;
	int dy = target_pos.y - unit->pos.y;

	t_pos next_pos = { unit->pos.x, unit->pos.y };
	if (abs(dx) > abs(dy))
	{
		next_pos.x += (dx > 0) ? 1 : -1;
	}
	else
	{
		next_pos.y += (dy > 0) ? 1 : -1;
	}

	t_obj *next_pos_obj = core_get_obj_from_pos(next_pos);
	if (next_pos_obj)
	{
		// If avoiding money and next position has money, try alternative path
		if (avoid_money && next_pos_obj->type == OBJ_MONEY)
		{
			t_pos alt_pos = try_alternative_move(unit->pos, dx, dy);
			t_obj *alt_obj = core_get_obj_from_pos(alt_pos);
			
			if (!alt_obj || (alt_obj->type != OBJ_MONEY))
			{
				core_action_move(unit, alt_pos);
				return;
			}
			// If both paths have money, just stay put
			return;
		}
		
		if (should_attack_object(unit, next_pos_obj))
		{
			core_action_attack(unit, next_pos);
		}
		else if (next_pos_obj->type == OBJ_WALL || 
		         (next_pos_obj->type == OBJ_UNIT && next_pos_obj->s_unit.team_id == game.my_team_id) ||
		         (next_pos_obj->type == OBJ_RESOURCE && unit->s_unit.unit_type != UNIT_MINER))
		{
			// Try alternative path for walls, friendly units, and resources (for non-miners)
			t_pos alt_pos = try_alternative_move(unit->pos, dx, dy);
			
			// If alternative found a valid move and doesn't conflict with money avoidance
			if (alt_pos.x != unit->pos.x || alt_pos.y != unit->pos.y)
			{
				t_obj *alt_obj = core_get_obj_from_pos(alt_pos);
				if (!alt_obj || !(avoid_money && alt_obj->type == OBJ_MONEY))
				{
					core_action_move(unit, alt_pos);
				}
				// Stay put if alternative has money and we're avoiding it
			}
			else if (next_pos_obj->type == OBJ_WALL)
			{
				core_action_attack(unit, next_pos); // Attack wall if can't go around
			}
			// Stay put if can't find alternative and can't attack
		}
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

bool is_valid_position(int x, int y)
{
	return (x >= 0 && x < 20 && y >= 0 && y < 20);
}

t_pos try_alternative_move(t_pos unit_pos, int dx, int dy)
{
	// Try multiple alternative directions when blocked
	t_pos alternatives[6];
	int alt_count = 0;
	
	// Add perpendicular directions first
	if (abs(dx) > abs(dy))
	{
		// Moving horizontally, try vertical moves
		if (dy > 0 && is_valid_position(unit_pos.x, unit_pos.y + 1)) {
			alternatives[alt_count++] = (t_pos){unit_pos.x, unit_pos.y + 1};
		} else if (dy < 0 && is_valid_position(unit_pos.x, unit_pos.y - 1)) {
			alternatives[alt_count++] = (t_pos){unit_pos.x, unit_pos.y - 1};
		} else {
			// No y preference, try both
			if (is_valid_position(unit_pos.x, unit_pos.y + 1)) {
				alternatives[alt_count++] = (t_pos){unit_pos.x, unit_pos.y + 1};
			}
			if (is_valid_position(unit_pos.x, unit_pos.y - 1)) {
				alternatives[alt_count++] = (t_pos){unit_pos.x, unit_pos.y - 1};
			}
		}
	}
	else
	{
		// Moving vertically, try horizontal moves  
		if (dx > 0 && is_valid_position(unit_pos.x + 1, unit_pos.y)) {
			alternatives[alt_count++] = (t_pos){unit_pos.x + 1, unit_pos.y};
		} else if (dx < 0 && is_valid_position(unit_pos.x - 1, unit_pos.y)) {
			alternatives[alt_count++] = (t_pos){unit_pos.x - 1, unit_pos.y};
		} else {
			// No x preference, try both
			if (is_valid_position(unit_pos.x + 1, unit_pos.y)) {
				alternatives[alt_count++] = (t_pos){unit_pos.x + 1, unit_pos.y};
			}
			if (is_valid_position(unit_pos.x - 1, unit_pos.y)) {
				alternatives[alt_count++] = (t_pos){unit_pos.x - 1, unit_pos.y};
			}
		}
	}
	
	// Add diagonal options as backup
	if (dx != 0 && dy != 0 && alt_count < 4) {
		int new_x = unit_pos.x + ((dx > 0) ? 1 : -1);
		int new_y = unit_pos.y + ((dy > 0) ? 1 : -1);
		if (is_valid_position(new_x, new_y)) {
			alternatives[alt_count++] = (t_pos){new_x, new_y};
		}
	}
	
	// Try each alternative in order, return first free space
	for (int i = 0; i < alt_count; i++)
	{
		t_pos alt_pos = alternatives[i];
		t_obj *alt_obj = core_get_obj_from_pos(alt_pos);
		if (!alt_obj)
		{
			return alt_pos;
		}
	}
	
	// If all alternatives blocked, return original position (stay put)
	return unit_pos;
}

bool should_attack_object(t_obj *unit, t_obj *target)
{
	// Money - always move
	if (target->type == OBJ_MONEY)
		return false;
	
	// Resources - miners mine them, others move
	if (target->type == OBJ_RESOURCE)
		return (unit->s_unit.unit_type == UNIT_MINER);
	
	// Enemy units and cores - attack
	if (target->type == OBJ_UNIT && target->s_unit.team_id != game.my_team_id)
		return true;
	if (target->type == OBJ_CORE && target->s_core.team_id != game.my_team_id)
		return true;
	
	return false;
}