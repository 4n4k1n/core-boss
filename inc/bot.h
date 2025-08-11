#ifndef BOT_H
#define BOT_H

#include "core_lib.h"

t_obj *ft_get_core_own(void);
t_obj *ft_get_core_opponent(void);
t_obj *ft_get_resource_nearest(t_pos pos);
t_obj *ft_get_money_nearest(t_pos pos);
t_obj *ft_get_resource_money_nearest(t_pos pos);
t_obj *ft_get_units_opponent_nearest(t_pos pos);
t_obj **ft_get_units_own(void);
t_obj **ft_get_units_opponent(void);
int ft_count_resources(void);
int ft_count_miners_own(void);
int ft_count_miners_opponent(void);
t_obj **ft_get_all_resources(void);
double ft_calculate_distance(t_pos pos1, t_pos pos2);
t_obj *ft_find_nearest_unassigned_resource(t_pos miner_pos, t_obj **all_miners);

#endif /* BOT_H */
