# Embeds src/storage/Schema.sql into a generated header as a raw string literal.
# Usage: cmake -DINPUT=<sql> -DOUTPUT=<header> -P EmbedSchema.cmake
file(READ ${INPUT} SCHEMA_SQL)
file(WRITE ${OUTPUT}.tmp
    "#pragma once\n\n"
    "// Generated from src/storage/Schema.sql — do not edit.\n"
    "namespace garageplaymate {\n"
    "inline constexpr const char* kSchemaSql = R\"schema_sql(${SCHEMA_SQL})schema_sql\";\n"
    "}  // namespace garageplaymate\n")
file(COPY_FILE ${OUTPUT}.tmp ${OUTPUT} ONLY_IF_DIFFERENT)
file(REMOVE ${OUTPUT}.tmp)
