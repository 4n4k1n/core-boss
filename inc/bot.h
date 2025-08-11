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

#endif /* BOT_H */
