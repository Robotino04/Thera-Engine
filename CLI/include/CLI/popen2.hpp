#pragma once

#include <unistd.h>

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <utility>

#include <memory>

using UniqueFile = std::unique_ptr<FILE, decltype(&fclose)>;
// from https://stackoverflow.com/a/64359731
// Like popen(), but returns two FILE*: child's stdin and stdout, respectively.
inline std::pair<UniqueFile, UniqueFile> popen2(const char *command){
    // pipes[0]: parent writes, child reads (child's stdin)
    // pipes[1]: child writes, parent reads (child's stdout)
    int pipes[2][2];


    if (pipe(pipes[0]) == -1)
        throw std::runtime_error("Cannot open pipe!");
    if (pipe(pipes[1]) == -1)
        throw std::runtime_error("Cannot open pipe!");
        
    if (fork() > 0){
        // parent
        close(pipes[0][0]);
        close(pipes[1][1]);

        return std::make_pair(
            UniqueFile(fdopen(pipes[0][1], "w"), fclose),
            UniqueFile(fdopen(pipes[1][0], "r"), fclose)
        );
    }
    else{
        // child
        close(pipes[0][1]);
        close(pipes[1][0]);

        dup2(pipes[0][0], STDIN_FILENO);
        dup2(pipes[1][1], STDOUT_FILENO);

        execl("/bin/sh", "/bin/sh", "-c", command, NULL);

        exit(1);
    }
}