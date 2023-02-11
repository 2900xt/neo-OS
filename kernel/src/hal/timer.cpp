double s_since_boot;

double get_boot_time()
{
    return s_since_boot;
}

extern "C" void inc_boot_time()
{
    s_since_boot += 0.001;
}