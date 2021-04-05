#include "mocks/mock_changer.h"

#include <CppUTestExt/MockSupport_c.h>

bool mock_changer(struct arg *arg, struct changer *c)
{
        char const *mock_changer_id = c->data;
        return (mock_c()->actualCall(__func__)
                        ->withStringParameters("id", mock_changer_id)
                        ->withIntParameters("type", arg->type)
                        ->withStringParameters("normal_arg", arg->normal.arg)
                        ->withUnsignedLongIntParameters("normal_opts_len", arg->normal.opts_len)
                        ->withPointerParameters("normal_opts", arg->normal.opts)
                        ->returnBoolValueOrDefault(true));
}
