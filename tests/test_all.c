int run_sensors(void);
int run_light(void);
int run_policy(void);
int run_commands(void);
int run_acceptance(void);
int main(void)
{
    return run_sensors() || run_light() || run_policy() || run_commands() || run_acceptance();
}

