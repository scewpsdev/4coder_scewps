/*
4coder_config.cpp - Parsing *.4coder files.
*/

// TOP

////////////////////////////////
// NOTE(allen): Config Variables Read

function u64
def_get_config_u64(Application_Links *app, String_ID key, u64 default_value) {
    Scratch_Block scratch(app);
    Variable_Handle var = def_get_config_var(key);
    u64 result = vars_is_nil(var) ? default_value : vars_u64_from_var(app, var);
    return(result);
}

// BOTTOM

