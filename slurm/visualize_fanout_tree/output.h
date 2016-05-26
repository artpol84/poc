#ifndef OUTPUT_GRAPH_H
#define OUTPUT_GRAPH_H
    typedef void (*add_launch)(char *buf, char *who, char *child_list);
    void add_launch_std(char *buf, char *who, char *child_list);
    void add_launch_dot(char *buf, char *who, char *child_list);
#endif