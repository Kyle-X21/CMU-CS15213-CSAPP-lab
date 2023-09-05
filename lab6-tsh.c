/* 
 * tsh - A tiny shell program with job control
 * 
 * I only retained the function codes that needed to be written for the lab.
 */
/* Here are the functions that you will implement */
void eval(char *cmdline);
int builtin_cmd(char **argv);
void do_bgfg(char **argv);
void waitfg(pid_t pid);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);
/*-----------------------------*/


/* 
 * eval - Evaluate the command line that the user has just typed in
 * 
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately. Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.  
*/
void eval(char *cmdline) 
{
    char* argv[MAXARGS];//参数
    char buf[MAXLINE];
    int bg=0;//需要一个前台还是后台作业
    int state=0;//指示运行状态
    pid_t pid=0;//子进程pid
    strcpy(buf, cmdline);//先保存原命令行字符串。不直接修改原串是一个好的编程习惯。
    bg = parseline(buf, argv);//这个函数已经写好了，命令行参数放在argv里
    //bg里指示了当前指令需要的是前台还是后台作业
    state = bg ? BG : FG;
    if (argv[0] == NULL)//说明输入了一个空行
        return;
    sigset_t mask_all, mask_one, prev_one;
    sigfillset(&mask_all);//把所有信号都添加到信号集合
    sigemptyset(&mask_one);//初始化一个空集合
    sigaddset(&mask_one, SIGCHLD);//把SIGCHLD信号加入集合
    if (!builtin_cmd(argv))//如果不是内置命令，创建一个子进程来执行可执行文件
    {
        sigprocmask(SIG_BLOCK, &mask_one, &prev_one);//阻塞SIGCHLD信号
        if ((pid = fork()) == 0)
        {
            //对于子进程
            sigprocmask(SIG_SETMASK, &prev_one, NULL);//把阻塞集合恢复
            setpgid(0, 0);//把子进程放到以自己的pid为gid的进程组中
            if (execve(argv[0], argv, environ) < 0)
                printf("Command not found.\n");//可执行文件路径一定是第一个参数
            exit(0);//子进程执行完就可以结束了
        }
        //对于父进程
        sigprocmask(SIG_BLOCK, &mask_all, NULL);//先阻塞所有信号，
        //因为我们不希望在addjob的时候程序去处理信号，否则可能会导致竞争
        addjob(jobs, pid, state, cmdline);//把作业放到作业列表里
        sigprocmask(SIG_SETMASK, &mask_one, NULL);//把阻塞恢复成阻塞SIGCHLD信号
        if (state == FG)
        {
            //需要的是前台作业
            waitfg(pid);//等待前台作业完成
        }
        else
        {
            printf("[%d] (%d) %s",pid2jid(pid), pid, cmdline);
        }
        sigprocmask(SIG_SETMASK, &prev_one, NULL);

    }
    //如果是内置命令，在builtin_cmd函数中执行
    return;
}

/* 
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.  
 */
int builtin_cmd(char **argv) 
{
    if (!strcmp(argv[0], "quit"))//如果是quit命令，直接终止程序
        exit(0);
    if (!strcmp(argv[0], "jobs"))//如果是jobs命令，调用listjobs
    {
        listjobs(jobs);
        return 1;
    }
    if (!strcmp(argv[0], "bg")|| !strcmp(argv[0], "fg"))
    {//如果是bg或fg，调用相应的处理函数执行命令
        do_bgfg(argv);
        return 1;
    }
    return 0;     /* not a builtin command */
}

/* 
 * do_bgfg - Execute the builtin bg and fg commands
 */
void do_bgfg(char **argv) 
{
    struct job_t* job = NULL;//要处理的job
    int state;//是BG还是FG
    int id;//存这个指令的参数
    if (!strcmp(argv[0], "bg"))state = BG;
    else state = FG;
    if (argv[1] == NULL)//没有参数
    {
        printf("fg or bg command requires PID or %%jobid argument\n");
        return;
    }
    if (argv[1][0] == '%')
    {//是jid
        if (sscanf(&argv[1][1], "%d", &id) < 1)
        {
            printf("fg or bg: argument must be a PID or %%jobid\n");
            return;
        }
        job = getjobjid(jobs, id);
    }
    else
    {//是pid
        if (sscanf(&argv[1][0], "%d", &id) < 1)
        {
            printf("fg or bg: argument must be a PID or %%jobid\n");
            return;
        }
        job = getjobpid(jobs, id);
    }
    if (job == NULL)//没有用户要的job
    {
        printf("(%d): No such process or job\n", id);
        return;
    }
    kill(-(job->pid), SIGCONT);//把相应的作业变成运行态
    job->state = state;
    if (state == FG)//如果是FG命令
    {
        waitfg(job->pid);
    }
    else
        printf("[%d] (%d) %s", job->jid, job->pid, job->cmdline);
        
    return;
}

/* 
 * waitfg - Block until process pid is no longer the foreground process
 */
void waitfg(pid_t pid)
{
    sigset_t mask_all,mask, mask_one, prev_one;
    sigfillset(&mask_all);//把所有信号都添加到信号集合
    sigemptyset(&mask);//初始化一个空集合
    sigemptyset(&mask_one);
    sigaddset(&mask_one, SIGCHLD);//把SIGCHLD信号加入集合
    sigprocmask(SIG_BLOCK, &mask_one, &prev_one);
    while (fgpid(jobs) != 0)
        sigsuspend(&mask);//显式地等待信号，如此写的理由见书8.5.7
    return;
}

/*****************
 * Signal handlers
 *****************/

/* 
 * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 *     a child job terminates (becomes a zombie), or stops because it
 *     received a SIGSTOP or SIGTSTP signal. The handler reaps all
 *     available zombie children, but doesn't wait for any other
 *     currently running children to terminate.  
 */
void sigchld_handler(int sig) 
{
    //这个处理函数回收所有当前的僵死进程，不要等待任何还在运行的进程终止
    int olderrno = errno;//见书p536，编写信号处理函数时都要注意保存和恢复errno，它是全局变量
    int status;
    pid_t pid;
    struct job_t* job;
    sigset_t mask, prev;
    sigfillset(&mask);
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0)
    {/*等待集合是所有的子进程
        等待行为是，如果任何子进程没有终止，
        立即返回0；如果有进程已经终止，就返回终止进程的PID*/
        sigprocmask(SIG_BLOCK, &mask, &prev);//先阻塞所有信号
        if (WIFEXITED(status))
        {//这是一个和waitpid一起使用的宏。判断子进程是否是正常退出。
            deletejob(jobs, pid);
        }
        else if (WIFSIGNALED(status))
        {
            //这个宏判断是否因为未被捕获的信号而终止
            printf("Job [%d] (%d) terminated by signal %d\n", pid2jid(pid), pid, WTERMSIG(status));
            deletejob(jobs, pid);
        }
        else if (WIFSTOPPED(status))
        {
            //这个宏判断子进程是否因为捕获到的信号而发出SIGCHLD信号，
            //比如SIGTSTP信号，作用就是暂时阻塞进程。
            printf("Job [%d] (%d) stopped by signal %d\n", pid2jid(pid), pid, WSTOPSIG(status));
            job = getjobpid(jobs, pid);
            job->state = ST;
        }
        sigprocmask(SIG_SETMASK, &prev, NULL);
    }
    errno = olderrno;
    return;
}

/* 
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.  
 */
void sigint_handler(int sig) 
{
    int olderrno = errno;
    int pid;
    sigset_t mask, prev;
    sigfillset(&mask);
    sigprocmask(SIG_BLOCK, &mask, &prev);//先阻塞所有信号
    if ((pid = fgpid(jobs)) != 0)
    {
        sigprocmask(SIG_SETMASK, &prev, NULL);
        kill(-pid, SIGINT);//用kill函数发出信号
    }
    errno = olderrno;
    return;
}

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending it a SIGTSTP.  
 */
void sigtstp_handler(int sig) 
{
    int olderrno = errno;
    int pid;
    sigset_t mask, prev;
    sigfillset(&mask);
    sigprocmask(SIG_BLOCK, &mask, &prev);//先阻塞所有信号
    if ((pid = fgpid(jobs)) != 0)
    {
        sigprocmask(SIG_SETMASK, &prev, NULL);
        kill(-pid, SIGTSTP);//用kill函数发出信号
    }
    errno = olderrno;
    return;
}

/*********************
 * End signal handlers
 *********************/
