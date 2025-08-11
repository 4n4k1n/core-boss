#include "bot.h"

#include <time.h>
#include <stdio.h>

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

	// Control all miners
	t_obj **units = ft_get_units_own();
	for (int i = 0; units && units[i]; i++)
	{
		t_obj *miner = units[i];
		if (miner->state != STATE_ALIVE || miner->s_unit.unit_type != UNIT_MINER)
			continue;

		// If miner has no money, go to nearest resource
		if (miner->s_unit.balance <= 0)
		{
			t_obj *nearest_resource = ft_get_resource_money_nearest(miner->pos);
			if (nearest_resource)
				move_and_attack(miner, nearest_resource->pos);
		}
		// If miner has money, return to core and transfer it
		else
		{
			move_and_attack(miner, core_own->pos);
			core_action_transferMoney(miner, core_own->pos, miner->s_unit.balance);
		}
	}
	free(units);
}