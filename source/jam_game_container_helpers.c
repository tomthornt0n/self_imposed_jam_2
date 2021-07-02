
#define ForInRange(_i, _a, _b) i32 _i = (_a); _i < (_b); _i += 1
#define ForCount(_a) ForInRange(_i, 0, _a)

//~NOTE(tbt): array backed stack

#define StackPush(_stack, _value) (_stack)[(_stack##_count)++] = (_value)

#define StackPop(_result, _stack) (_stack)[--(_stack##_count)]

#define StackForEach(_type, _current, _stack) \
_type *_current = (_stack);           \
_current - (_stack) < (_stack##_count);   \
_current += 1

//~NOTE(tbt): singly linked list

#define LLPush(_list, _value) \
(_value)->next = (_list);   \
(_list) = (_value)

#define LLForEach(_type, _current, _list) \
_type *_current = (_list);              \
NULL != _current;                       \
_current = _current->next