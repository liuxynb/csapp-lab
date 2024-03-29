/*
 * tsh - A tiny shell program with job control
 *
 * <Put your name and login ID here>
 */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

/* Misc manifest constants */
#define MAXLINE 1024   /* max line size */
#define MAXARGS 128    /* max args on a command line */
#define MAXJOBS 16     /* max jobs at any point in time */
#define MAXJID 1 << 16 /* max job ID */

/* Job states */
#define UNDEF 0 /* undefined */
#define FG 1    /* running in foreground */
#define BG 2    /* running in background */
#define ST 3    /* stopped */

/*
 * Jobs states: FG (foreground), BG (background), ST (stopped)
 * Job state transitions and enabling actions:
 *     FG -> ST  : ctrl-z
 *     ST -> FG  : fg command
 *     ST -> BG  : bg command
 *     BG -> FG  : fg command
 * At most 1 job can be in the FG state.
 */

/* Global variables */
extern char **environ;   /* defined in libc */
char prompt[] = "tsh> "; /* command line prompt (DO NOT CHANGE) */
int verbose = 0;         /* if true, print additional output */
int nextjid = 1;         /* next job ID to allocate */
char sbuf[MAXLINE];      /* for composing sprintf messages */

struct job_t
{                          /* The job struct */
    pid_t pid;             /* job PID */
    int jid;               /* job ID [1, 2, ...] */
    int state;             /* UNDEF, BG, FG, or ST */
    char cmdline[MAXLINE]; /* command line */
};
struct job_t jobs[MAXJOBS]; /* The job list */
/* End global variables */

/* Function prototypes */

/* Here are the functions that you will implement */
void eval(char *cmdline);
int builtin_cmd(char **argv);
void do_bgfg(char **argv);
void waitfg(pid_t pid);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);

/* Here are helper routines that we've provided for you */
int parseline(const char *cmdline, char **argv);
void sigquit_handler(int sig);

void clearjob(struct job_t *job);
void initjobs(struct job_t *jobs);
int maxjid(struct job_t *jobs);
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline);
int deletejob(struct job_t *jobs, pid_t pid);
pid_t fgpid(struct job_t *jobs);
struct job_t *getjobpid(struct job_t *jobs, pid_t pid);
struct job_t *getjobjid(struct job_t *jobs, int jid);
int pid2jid(pid_t pid);
void listjobs(struct job_t *jobs);

void usage(void);
void unix_error(char *msg);
void app_error(char *msg);
typedef void handler_t(int);
handler_t *Signal(int signum, handler_t *handler);

/*
 * main - The shell's main routine
 */
int main(int argc, char **argv)
{
    char c;
    char cmdline[MAXLINE];
    int emit_prompt = 1; /* emit prompt (default) */

    /* Redirect stderr to stdout (so that driver will get all output
     * on the pipe connected to stdout) */
    dup2(1, 2);

    /* Parse the command line */
    while ((c = getopt(argc, argv, "hvp")) != EOF)
    {
        switch (c)
        {
        case 'h': /* print help message */
            usage();
            break;
        case 'v': /* emit additional diagnostic info */
            verbose = 1;
            break;
        case 'p':            /* don't print a prompt */
            emit_prompt = 0; /* handy for automatic testing */
            break;
        default:
            usage();
        }
    }

    /* Install the signal handlers */

    /* These are the ones you will need to implement */
    Signal(SIGINT, sigint_handler);   /* ctrl-c */
    Signal(SIGTSTP, sigtstp_handler); /* ctrl-z */
    Signal(SIGCHLD, sigchld_handler); /* Terminated or stopped child */

    /* This one provides a clean way to kill the shell */
    Signal(SIGQUIT, sigquit_handler);

    /* Initialize the job list */
    initjobs(jobs);

    /* Execute the shell's read/eval loop */
    while (1)
    {

        /* Read command line */
        if (emit_prompt)
        {
            printf("%s", prompt);
            fflush(stdout);
        }
        if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
            app_error("fgets error");
        if (feof(stdin))
        { /* End of file (ctrl-d) */
            fflush(stdout);
            exit(0);
        }

        /* Evaluate the command line */
        eval(cmdline);
        fflush(stdout);
        fflush(stdout);
    }

    exit(0); /* control never reaches here */
}

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
    // char *argv[MAXARGS];
    // char buf[MAXLINE]; // the buffer of cmd line
    // int bg;            // background
    // pid_t pid;
    // sigset_t mask_all, prev_all;
    // strcpy(buf, cmdline);
    // bg = parseline(cmdline, argv);
    // if (argv[0] == NULL)
    //     return;
    // if (builtin_cmd(argv)) // If the user has requested a built-in command (quit, jobs, bg or fg) then execute it immediately.
    //     return;
    // sigfillset(&mask_all);                        // blocks all signals by setting the signal mask to include all signals using sigfillset
    // sigprocmask(SIG_BLOCK, &mask_all, &prev_all); // blocking.

    // if ((pid = fork()) == 0) // Child process
    // {
    //     sigprocmask(SIG_SETMASK, &prev_all, NULL);
    //     setpgid(0, 0);
    //     if (execve(argv[0], argv, environ) < 0)
    //     {
    //         printf("%s: Command not found.\n", argv[0]);
    //         exit(0);
    //     }
    // }
    // // parent process
    // if (!bg)
    // {
    //     addjob(jobs, pid, FG, cmdline);
    //     sigprocmask(SIG_SETMASK, &prev_all, NULL); // 恢复先前的信号
    //     waitfg(pid);                               // Block until process pid is no longer the foreground process
    // }
    // else
    // {
    //     addjob(jobs, pid, BG, cmdline);
    //     sigprocmask(SIG_SETMASK, &prev_all, NULL);
    //     printf("[%d] (%d) %s", pid2jid(pid), pid, cmdline);
    // }
    // return;
    char *argv[MAXARGS];
    char buf[MAXLINE];
    int bg;
    pid_t pid;
    sigset_t mask_all, prev_all;

    strcpy(buf, cmdline);
    bg = parseline(cmdline, argv);
    if (argv[0] == NULL)
        return;

    if (builtin_cmd(argv))
        // 修改了判断条件——避免 if 语句嵌套
        return;

    sigfillset(&mask_all);
    sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
    // 加上了阻塞信号的代码

    if ((pid = fork()) == 0) // 子进程
    {
        sigprocmask(SIG_SETMASK, &prev_all, NULL);
        setpgid(0, 0);
        // 对 setpgid 的调用；下面会介绍它

        if (execve(argv[0], argv, environ) < 0)
        {
            printf("%s: Command not found.\n", argv[0]);
            exit(0);
        }
    }

    // 子进程不会执行到这里
    if (!bg)
    {
        addjob(jobs, pid, FG, cmdline);
        // 添上去的一行——向全局作业表中添加作业
        sigprocmask(SIG_SETMASK, &prev_all, NULL);
        waitfg(pid);
        // 等待前台进程结束；waitfg 我们一会儿就写
    }
    else // 后台进程的情况
    {
        addjob(jobs, pid, BG, cmdline);
        sigprocmask(SIG_SETMASK, &prev_all, NULL);
        // 这里就不等子进程了——收拾收拾就返回
        printf("[%d] (%d) %s", pid2jid(pid), pid, cmdline);
        // 输出相关信息——就像 bash 那样
    }
}

/*
 * parseline - Parse the command line and build the argv array.
 *
 * Characters enclosed in single quotes are treated as a single
 * argument.  Return true if the user has requested a BG job, false if
 * the user has requested a FG job.
 */
int parseline(const char *cmdline, char **argv)
{
    static char array[MAXLINE]; /* holds local copy of command line */
    char *buf = array;          /* ptr that traverses command line */
    char *delim;                /* points to first space delimiter */
    int argc;                   /* number of args */
    int bg;                     /* background job? */

    strcpy(buf, cmdline);
    buf[strlen(buf) - 1] = ' ';   /* replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* ignore leading spaces */
        buf++;

    /* Build the argv list */
    argc = 0;
    if (*buf == '\'')
    {
        buf++;
        delim = strchr(buf, '\'');
    }
    else
    {
        delim = strchr(buf, ' ');
    }

    while (delim)
    {
        argv[argc++] = buf;
        *delim = '\0';
        buf = delim + 1;
        while (*buf && (*buf == ' ')) /* ignore spaces */
            buf++;

        if (*buf == '\'')
        {
            buf++;
            delim = strchr(buf, '\'');
        }
        else
        {
            delim = strchr(buf, ' ');
        }
    }
    argv[argc] = NULL;

    if (argc == 0) /* ignore blank line */
        return 1;

    /* should the job run in the background? */
    if ((bg = (*argv[argc - 1] == '&')) != 0)
    {
        argv[--argc] = NULL;
    }
    return bg;
}

/*
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.
 */
int builtin_cmd(char **argv)
{
    // if (strcmp(argv[0], "quit") == 0)
    // {
    //     exit(0);
    // }
    // else if (strcmp(argv[0], "jobs") == 0)
    // {
    //     listjobs(jobs); // print the jobs
    //     return 1;
    // }
    // else if (strcmp(argv[0], "fg") == 0 || strcmp(argv[0], "bg") == 0)
    // {
    //     do_bgfg(argv); // Execute the builtin bg and fg commands
    //     return 1;
    // }
    // else if (strcmp(argv[0], "&") == 0)
    // {
    //     return 1;
    // }
    // else
    //     return 0; /* not a builtin command */
    if (strcmp(argv[0], "quit") == 0)
        exit(0);
    else if (strcmp(argv[0], "jobs") == 0)
    {
        listjobs(jobs);
        // 借用作者给我们写好的 listjobs
        return 1;
    }
    else if (strcmp(argv[0], "fg") == 0 || strcmp(argv[0], "bg") == 0)
    {
        do_bgfg(argv);
        // 这个函数是专门处理 bg 和 fg 命令的，下面我们会提到它
        return 1;
    }
    else if (strcmp(argv[0], "&") == 0)
        // 不处理单独的 '&'
        return 1;
    return 0; /* not a builtin command */
}

/*
 * do_bgfg - Execute the builtin bg and fg commands
 */
void do_bgfg(char **argv)
{
    // if (argv[1] == NULL)
    // {
    //     printf("%s command requires PID or %%jobid argument\n", argv[0]);
    //     return;
    // }
    // sigset_t mask_all, prev_all;
    // sigfillset(&mask_all);
    // sigprocmask(SIG_BLOCK, &mask_all, &prev_all); // block all the signal
    // struct job_t *job;
    // int jid = 0, pid = 0;
    // if (argv[1][0] == '%')
    // {
    //     jid = atoi(argv[1] + 1);
    // }
    // else
    // {
    //     pid = atoi(argv[1]);
    // }
    // // 填写错误的情况
    // if ((jid == 0 && argv[1][0] == '%') || (pid == 0 && argv[1][0] != '%'))
    // {
    //     printf("%s: argument must be a PID or %%jobid\n", argv[0]);
    //     sigprocmask(SIG_SETMASK, &prev_all, NULL);
    //     return;
    // }
    // if (jid == 0)
    // {
    //     jid = pid2jid(pid);
    // }
    // job = getjobjid(jobs, jid);
    // // job do not exist
    // if (job == NULL)
    // {
    //     if (argv[1][0] != '%')
    //         printf("(%s): ", argv[1]);
    //     else
    //         printf("%s: ", argv[1]);
    //     printf("No such %s\n", argv[1][0] == '%' ? "job" : "process");
    //     sigprocmask(SIG_SETMASK, &prev_all, NULL);
    //     return;
    // }

    // if (strcmp(argv[0], "bg") == 0)
    // {
    //     job->state = BG;
    //     printf("[%d] (%d) %s", job->jid, job->pid, job->cmdline);
    //     kill(-(job->pid), SIGCONT);
    //     sigprocmask(SIG_SETMASK, &prev_all, NULL);
    // }
    // else
    // {
    //     if (job->state == ST)
    //     {
    //         kill(-(job->pid), SIGCONT);
    //     }
    //     job->state = FG;
    //     sigprocmask(SIG_SETMASK, &prev_all, NULL);
    //     waitfg(job->pid);
    // }
    // return;
    if (argv[1] == NULL)
    // 没有额外参数的情况
    {
        printf("%s command requires PID or %%jobid argument\n", argv[0]);
        return;
    }

    sigset_t mask_all, prev_all;
    sigfillset(&mask_all);
    sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
    // 阻塞信号——下面要访问全局数组了

    struct job_t *job;
    int jid = 0, pid = 0;

    // 获取 jid 或是 pid
    if (argv[1][0] == '%')
        jid = atoi(argv[1] + 1);
    else
        pid = atoi(argv[1]);

    // 乱填 jid 或是 pid 的情况之一
    if ((jid == 0 && argv[1][0] == '%') || (pid == 0 && argv[1][0] != '%'))
    {
        printf("%s: argument must be a PID or %%jobid\n", argv[0]);
        sigprocmask(SIG_SETMASK, &prev_all, NULL);
        return;
    }

    if (jid == 0)
        jid = pid2jid(pid);
    job = getjobjid(jobs, jid);

    // 乱填 jid 或是 pid 的情况之二
    if (job == NULL)
    {
        if (argv[1][0] != '%')
            printf("(%s): ", argv[1]);
        else
            printf("%s: ", argv[1]);
        printf("No such %s\n", argv[1][0] == '%' ? "job" : "process");

        sigprocmask(SIG_SETMASK, &prev_all, NULL);
        return;
    }

    // 最重要的部分就是下面两块了
    if (strcmp(argv[0], "bg") == 0)
    {
        job->state = BG;
        printf("[%d] (%d) %s", job->jid, job->pid, job->cmdline);
        kill(-(job->pid), SIGCONT);
        sigprocmask(SIG_SETMASK, &prev_all, NULL);
    }
    else
    {
        if (job->state == ST)
            kill(-(job->pid), SIGCONT);
        job->state = FG;
        sigprocmask(SIG_SETMASK, &prev_all, NULL);
        waitfg(job->pid);
    }
}

/*
 * waitfg - Block until process pid is no longer the foreground process
 */
void waitfg(pid_t pid)
{
    // sigset_t mask_all, prev_all;
    // sigfillset(&mask_all);
    // while (1)
    // {
    //     sigprocmask(SIG_BLOCK, &mask_all, &prev_all); // block the signals when access the global variable.
    //     struct job_t *foreground_job = getjobpid(jobs, pid);
    //     sigprocmask(SIG_SETMASK, &prev_all, NULL);
    //     if (!foreground_job || foreground_job->state != FG)
    //     {
    //         break;
    //     }
    //     sleep(1);
    // }
    // return;
    sigset_t mask_all, prev_all;
    sigfillset(&mask_all);

    while (1)
    {
        // 访问全局变量记得阻塞有关的信号呀~
        sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
        struct job_t *foreground_job = getjobpid(jobs, pid);
        sigprocmask(SIG_SETMASK, &prev_all, NULL);

        if (!foreground_job || foreground_job->state != FG)
            // 当找不到前台进程，或者前台进程已经挂起，就退出循环
            break;

        sleep(1);
    }
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
    // int olderrno = errno;
    // int status;
    // pid_t pid;
    // sigset_t mask_all, prev_all;
    // sigfillset(&mask_all);
    // while ((pid = waitpid(-1, &status, WUNTRACED | WNOHANG)) > 0) // If PID is (pid_t) -1, match any process.
    // {
    //     sigprocmask(SIG_BLOCK, &mask_all, &prev_all); // 阻塞所有信号
    //     if (WIFSIGNALED(status))
    //     {
    //         printf("Job [%d] (%d) terminated by signal %d\n", pid2jid(pid), pid, WTERMSIG(status));
    //         deletejob(jobs, pid);
    //     }
    //     else if (WIFSTOPPED(status))
    //     {
    //         printf("Job [%d] (%d) stopped by signal %d\n", pid2jid(pid), pid, WTERMSIG(status));
    //         struct job_t *job = getjobpid(jobs, pid);
    //         job->state = ST;
    //     }
    //     else
    //         deletejob(jobs, pid); // delete directly
    // }
    // sigprocmask(SIG_BLOCK, &prev_all, NULL);
    // errno = olderrno;
    // return;
    int olderrno = errno;
    // 存储 errno ——编写安全的信号处理函数要求之一
    int status;
    pid_t pid;
    sigset_t mask_all, prev_all;
    sigfillset(&mask_all);

    while ((pid = waitpid(-1, &status, WUNTRACED | WNOHANG)) > 0)
    // 为什么第三个参数不是 0 呢？原因我们下面会提到
    {
        sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
        // 阻塞所有信号——编写安全的信号处理函数要求之二

        if (WIFSIGNALED(status))
        // 子进程终止是 Ctrl + C 引发的么？
        {
            printf("Job [%d] (%d) terminated by signal %d\n",
                   pid2jid(pid), pid, WTERMSIG(status));
            // 按照 writeup 中的要求打印一条消息
            // csapp 里面的要求是，
            // 不在信号处理函数里面调用异步信号不安全的函数（比如 sprintf 和 printf）。
            // 但是，查一下 tshref 的反汇编，你会发现官方的实现在这里调用了 sprintf。
            // 可见，官方也没有认真遵守自己在书中提到的要求。
            deletejob(jobs, pid);
        }
        else if (WIFSTOPPED(status))
        // 子进程被 Ctrl + Z 阻塞了
        {
            printf("Job [%d] (%d) stopped by signal %d\n",
                   pid2jid(pid), pid, WSTOPSIG(status));
            struct job_t *job = getjobpid(jobs, pid);
            job->state = ST;
            // 改下子进程的状态就好，不要把它从列表中删除
        }
        else // 子进程寿终正寝的情况
            deletejob(jobs, pid);
        // 直接删除就好
    }

    sigprocmask(SIG_SETMASK, &prev_all, NULL);
    errno = olderrno;
}

/*
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.
 */
/*sigprocmask()是一个函数，用于检查或更改进程的信号屏蔽字。
信号屏蔽字是一个位掩码，指定了当前被阻塞的信号集合。
当信号被阻塞时，它们不会被传递给进程。
sigprocmask()函数可以用来更改信号屏蔽字，以便允许或阻止特定的信号。

SIG_SETMASK是一个宏，用于指定sigprocmask()函数的操作类型。
当将SIG_SETMASK作为参数传递给sigprocmask()函数时，
它将用新的信号屏蔽字替换当前的信号屏蔽字。
这意味着，之前被阻塞的信号现在将被解除阻塞，而新的信号将被阻塞
*/
void sigint_handler(int sig)
{
    // int olderrno = errno; // 暂存errno
    // sigset_t mask_all, prev_all;
    // sigfillset(&mask_all);
    // sigprocmask(SIG_BLOCK, &mask_all, &prev_all); // 阻塞信号
    // pid_t pid = fgpid(jobs);
    // if (pid == 0)
    // {
    //     errno = olderrno;
    //     sigprocmask(SIG_SETMASK, &prev_all, NULL);
    //     return;
    // }
    // kill(-pid, SIGINT); // 保证”转发“的信号能够被传递给整个子进程组
    // sigprocmask(SIG_SETMASK, &prev_all, NULL);
    // errno = olderrno;
    // return;
    int olderrno = errno;
    // 暂存 errno
    sigset_t mask_all, prev_all;
    sigfillset(&mask_all);
    sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
    // 阻塞所有信号
    pid_t pid = fgpid(jobs);
    if (pid == 0)
    {
        // 没有前台进程么？收拾收拾返回
        errno = olderrno;
        sigprocmask(SIG_SETMASK, &prev_all, NULL);
        return;
    }

    kill(-pid, SIGINT);
    sigprocmask(SIG_SETMASK, &prev_all, NULL);

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
    // int olderrno = errno;
    // sigset_t mask_all, prev_all;
    // sigfillset(&mask_all);
    // sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
    // pid_t pid = fgpid(jobs);
    // if (pid == 0)
    // {
    //     errno = olderrno;
    //     sigprocmask(SIG_SETMASK, &prev_all, NULL);
    //     return;
    // }
    // kill(-pid, SIGTSTP); // 如果将第一个参数设置为负数且绝对值不为1，则表示等待任何一个*进程组*ID等于该绝对值的子进程退出或停止。
    // sigprocmask(SIG_SETMASK, &prev_all, NULL);
    // errno = olderrno;
    // return;
    int olderrno = errno;

    sigset_t mask_all, prev_all;
    sigfillset(&mask_all);
    sigprocmask(SIG_BLOCK, &mask_all, &prev_all);

    pid_t pid = fgpid(jobs);
    if (pid == 0)
    {
        errno = olderrno;
        sigprocmask(SIG_SETMASK, &prev_all, NULL);
        return;
    }

    kill(-pid, SIGTSTP);
    sigprocmask(SIG_SETMASK, &prev_all, NULL);

    errno = olderrno;
}

/*********************
 * End signal handlers
 *********************/

/***********************************************
 * Helper routines that manipulate the job list
 **********************************************/

/* clearjob - Clear the entries in a job struct */
void clearjob(struct job_t *job)
{
    job->pid = 0;
    job->jid = 0;
    job->state = UNDEF;
    job->cmdline[0] = '\0';
}

/* initjobs - Initialize the job list */
void initjobs(struct job_t *jobs)
{
    int i;

    for (i = 0; i < MAXJOBS; i++)
        clearjob(&jobs[i]);
}

/* maxjid - Returns largest allocated job ID */
int maxjid(struct job_t *jobs)
{
    int i, max = 0;

    for (i = 0; i < MAXJOBS; i++)
        if (jobs[i].jid > max)
            max = jobs[i].jid;
    return max;
}

/* addjob - Add a job to the job list */
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline)
{
    int i;

    if (pid < 1)
        return 0;

    for (i = 0; i < MAXJOBS; i++)
    {
        if (jobs[i].pid == 0)
        {
            jobs[i].pid = pid;
            jobs[i].state = state;
            jobs[i].jid = nextjid++;
            if (nextjid > MAXJOBS)
                nextjid = 1;
            strcpy(jobs[i].cmdline, cmdline);
            if (verbose)
            {
                printf("Added job [%d] %d %s\n", jobs[i].jid, jobs[i].pid, jobs[i].cmdline);
            }
            return 1;
        }
    }
    printf("Tried to create too many jobs\n");
    return 0;
}

/* deletejob - Delete a job whose PID=pid from the job list */
int deletejob(struct job_t *jobs, pid_t pid)
{
    int i;

    if (pid < 1)
        return 0;

    for (i = 0; i < MAXJOBS; i++)
    {
        if (jobs[i].pid == pid)
        {
            clearjob(&jobs[i]);
            nextjid = maxjid(jobs) + 1;
            return 1;
        }
    }
    return 0;
}

/* fgpid - Return PID of current foreground job, 0 if no such job */
pid_t fgpid(struct job_t *jobs)
{
    int i;

    for (i = 0; i < MAXJOBS; i++)
        if (jobs[i].state == FG)
            return jobs[i].pid;
    return 0;
}

/* getjobpid  - Find a job (by PID) on the job list */
struct job_t *getjobpid(struct job_t *jobs, pid_t pid)
{
    int i;

    if (pid < 1)
        return NULL;
    for (i = 0; i < MAXJOBS; i++)
        if (jobs[i].pid == pid)
            return &jobs[i];
    return NULL;
}

/* getjobjid  - Find a job (by JID) on the job list */
struct job_t *getjobjid(struct job_t *jobs, int jid)
{
    int i;

    if (jid < 1)
        return NULL;
    for (i = 0; i < MAXJOBS; i++)
        if (jobs[i].jid == jid)
            return &jobs[i];
    return NULL;
}

/* pid2jid - Map process ID to job ID */
int pid2jid(pid_t pid)
{
    int i;

    if (pid < 1)
        return 0;
    for (i = 0; i < MAXJOBS; i++)
        if (jobs[i].pid == pid) // search
        {
            return jobs[i].jid;
        }
    return 0;
}

/* listjobs - Print the job list */
void listjobs(struct job_t *jobs)
{
    int i;

    for (i = 0; i < MAXJOBS; i++)
    {
        if (jobs[i].pid != 0)
        {
            printf("[%d] (%d) ", jobs[i].jid, jobs[i].pid);
            switch (jobs[i].state)
            {
            case BG:
                printf("Running ");
                break;
            case FG:
                printf("Foreground ");
                break;
            case ST:
                printf("Stopped ");
                break;
            default:
                printf("listjobs: Internal error: job[%d].state=%d ",
                       i, jobs[i].state);
            }
            printf("%s", jobs[i].cmdline);
        }
    }
}
/******************************
 * end job list helper routines
 ******************************/

/***********************
 * Other helper routines
 ***********************/

/*
 * usage - print a help message
 */
void usage(void)
{
    printf("Usage: shell [-hvp]\n");
    printf("   -h   print this message\n");
    printf("   -v   print additional diagnostic information\n");
    printf("   -p   do not emit a command prompt\n");
    exit(1);
}

/*
 * unix_error - unix-style error routine
 */
void unix_error(char *msg)
{
    fprintf(stdout, "%s: %s\n", msg, strerror(errno));
    exit(1);
}

/*
 * app_error - application-style error routine
 */
void app_error(char *msg)
{
    fprintf(stdout, "%s\n", msg);
    exit(1);
}

/*
 * Signal - wrapper for the sigaction function
 */
handler_t *Signal(int signum, handler_t *handler)
{
    struct sigaction action, old_action;

    action.sa_handler = handler;
    sigemptyset(&action.sa_mask); /* block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0)
        unix_error("Signal error");
    return (old_action.sa_handler);
}

/*
 * sigquit_handler - The driver program can gracefully terminate the
 *    child shell by sending it a SIGQUIT signal.
 */
void sigquit_handler(int sig)
{
    printf("Terminating after receipt of SIGQUIT signal\n");
    exit(1);
}
