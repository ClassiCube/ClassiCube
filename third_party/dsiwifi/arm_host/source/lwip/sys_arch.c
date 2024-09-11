#include "sys.h"

#include <nds.h>

//
// Misc
//

//int errno;

void __attribute__((weak)) sys_init(void)
{
}

//
// Time
//

static vu32 val = 0;

void sys_now_inc(u32 amt)
{
    val += amt;
}

u32_t
sys_now(void)
{
    return val;
}



//
// Threading
//

#define MBOX_VAL_EMPTY (0x12345678)
#define MBOX_VAL_INVALID (0x12345679)
#define SEM_SIGNAL (0x12345678)
#define MTX_LOCKED (0x12345678)

err_t __attribute__((weak)) sys_mutex_new(sys_mutex_t *mutex)
{
    *mutex = 0xFFFFFFFF;
    return ERR_OK;
}

void __attribute__((weak)) sys_mutex_lock(sys_mutex_t *mutex)
{
    if (!mutex) return;
    
    if (!*mutex)
        *mutex = 0xFFFFFFFF;

    //wifi_printlnf("lock %p %x %x", mutex, *mutex, getCPSR() & 0x1F);

    if ((getCPSR() & 0x1F) == 0x1F)
    {
        while (1)
        {
            if (*mutex == 0xFFFFFFFF) break;
        }
    }

    *mutex = MTX_LOCKED;
}

void __attribute__((weak)) sys_mutex_unlock(sys_mutex_t *mutex)
{
    if (!mutex) return;
    
    //wifi_printlnf("unlock %p %x %x", mutex, *mutex, getCPSR() & 0x1F);
    
    //if (*mutex == MTX_LOCKED)
    *mutex = 0xFFFFFFFF;
}

void __attribute__((weak)) sys_mutex_free(sys_mutex_t *mutex)
{
    if (!mutex) return;
    *mutex = 0;
}

int __attribute__((weak)) sys_mutex_valid(sys_mutex_t *mutex)
{
    return !!(mutex);
}

void __attribute__((weak)) sys_mutex_set_invalid(sys_mutex_t *mutex)
{
    if (!mutex) return;
    *mutex = 0;
}

err_t __attribute__((weak)) sys_sem_new(sys_sem_t *sem, u8_t count)
{
    *sem = 0xFFFFFFFF;
    return ERR_OK;
}

void __attribute__((weak)) sys_sem_signal(sys_sem_t *sem)
{
    //wifi_printf("sem signal %p...\n", sem);
    //while (*sem != 0xFFFFFFFF)
    {
        //wifi_printf("sem signal %p...\n", sem);
    }
    //wifi_printf("sem signal %p done\n", sem);
    *sem = SEM_SIGNAL;
}

u32_t __attribute__((weak)) sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{
    //wifi_printf("sem wait %p...\n", sem);
    if (!timeout)
    {
        //while (*sem != SEM_SIGNAL);
        timeout = 1000;
        while (timeout != 0)
        {
            if (*sem == SEM_SIGNAL) break;
            sys_msleep(1);
            timeout--;
        }
        
        if (*sem != SEM_SIGNAL)
            return SYS_ARCH_TIMEOUT;
    }
    else
    {
        while (timeout != 0)
        {
            if (*sem == SEM_SIGNAL) break;
            sys_msleep(1);
            timeout--;
        }

        if (*sem != SEM_SIGNAL)
            return SYS_ARCH_TIMEOUT;
    }

    //wifi_printf("sem got %p\n", sem);
    *sem = 0xFFFFFFFF;
    return 1;
}

void __attribute__((weak)) sys_sem_free(sys_sem_t *sem)
{
    *sem = 0;
}

int __attribute__((weak)) sys_sem_valid(sys_sem_t *sem)
{
    return !!(sem);
}

void __attribute__((weak)) sys_sem_set_invalid(sys_sem_t *sem)
{
    *sem = 0;
}

void __attribute__((weak)) sys_msleep(u32_t ms)
{
    vu32 target = val + ms;
    
    // edge case, overflow
    if (val > target) return;
    
    // Don't do this in an IRQ
    if ((getCPSR() & 0x1F) == 0x1F)
    {
        while (val < target) ;
    }
}

err_t __attribute__((weak)) sys_mbox_new(sys_mbox_t *mbox, int size)
{
    if (!mbox) return ERR_MEM;
    
    *mbox = MBOX_VAL_EMPTY;
    return ERR_OK;
}

void __attribute__((weak)) sys_mbox_post(sys_mbox_t *mbox, void *msg)
{
    if (!mbox || *mbox == MBOX_VAL_INVALID) return;

    while (*mbox != MBOX_VAL_EMPTY)
    {
        //wifi_printlnf("wait post");
    }
    
    /*if (*mbox == MBOX_VAL_INVALID)
    {
        wifi_printlnf("invalid invalid");
        *mbox = 0xAAAAAAAA;
        return;
    }*/
    
    *mbox = (uint32_t)msg;
}

err_t __attribute__((weak)) sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{
    if (!mbox || *mbox == MBOX_VAL_INVALID) return ERR_MEM;
    
    if (*mbox != MBOX_VAL_EMPTY)
    {
        //wifi_printlnf("invalid invalid 2--");
        //*mbox = 0xAAAAAAAA;
        return ERR_MEM;
    }
    
    //wifi_printlnf("put %x", msg);

    /*if (*mbox == MBOX_VAL_INVALID)
    {
        wifi_printlnf("invalid invalid 2");
        *mbox = 0xAAAAAAAA;
        return ERR_MEM;
    }*/
    
    *mbox = (uint32_t)msg;
    
    return ERR_OK;
}

err_t __attribute__((weak)) sys_mbox_trypost_fromisr(sys_mbox_t *mbox, void *msg)
{
    if (!mbox || *mbox == MBOX_VAL_INVALID) return ERR_MEM;

    if (*mbox != MBOX_VAL_EMPTY)
    {
        return ERR_MEM;
    }
    
    /*if (*mbox == MBOX_VAL_INVALID)
    {
        wifi_printlnf("invalid invalid 3");
        *mbox = 0xAAAAAAAA;
        return ERR_MEM;
    }*/
    
    *mbox = (uint32_t)msg;
    
    return ERR_OK;
}

u32_t __attribute__((weak)) sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout)
{
    if (!mbox || *mbox == MBOX_VAL_INVALID) return SYS_ARCH_TIMEOUT;
    *msg = NULL;

    if (!timeout)
    {
        while (*mbox == MBOX_VAL_EMPTY)
        {
            //wifi_printlnf("wait mbox %x", *mbox);
            ;
        }
    }
    else
    {
        while (timeout != 0)
        {
            if (*mbox != MBOX_VAL_EMPTY) break;
            sys_msleep(1);
            timeout--;
        }

        if (*mbox == MBOX_VAL_EMPTY)
            return SYS_ARCH_TIMEOUT;
    }
    
    *msg = (void*)(*mbox);
    *mbox = MBOX_VAL_EMPTY;

    //
    return 0;
}

u32_t __attribute__((weak)) sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg)
{
    if (!mbox || *mbox == MBOX_VAL_INVALID) return SYS_MBOX_EMPTY;
    
    if (*mbox == MBOX_VAL_EMPTY)
    {
        *msg = NULL;
        return SYS_MBOX_EMPTY;
    }
    
    *msg = (void*)(*mbox);
    *mbox = MBOX_VAL_EMPTY;

    return 0;
}

void __attribute__((weak)) sys_mbox_free(sys_mbox_t *mbox)
{
    if (!mbox) return;
    *mbox = MBOX_VAL_INVALID;
}

int __attribute__((weak)) sys_mbox_valid(sys_mbox_t *mbox)
{
    if (!mbox) return 0;
    
    return (*mbox != MBOX_VAL_INVALID);
}

void __attribute__((weak)) sys_mbox_set_invalid(sys_mbox_t *mbox)
{
    *mbox = MBOX_VAL_INVALID;
}

sys_thread_t __attribute__((weak)) sys_thread_new(const char *name, lwip_thread_fn thread, void *arg, int stacksize, int prio)
{
    return 0;
}
