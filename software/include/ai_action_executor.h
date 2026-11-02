#ifndef AI_ACTION_EXECUTOR_H
#define AI_ACTION_EXECUTOR_H

#ifdef __cplusplus
extern "C" {
#endif

int ai_action_execute(const char* json_action);
void ai_action_execute_sequence(const char* json_actions);
const char* ai_action_get_last_error(void);

#ifdef __cplusplus
}
#endif

#endif /* AI_ACTION_EXECUTOR_H */
