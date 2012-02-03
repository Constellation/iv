"TESTING".endsWith("ING") &&
"TESTING".endsWith("TEST", 4) &&
"TESTING".endsWith("NG", 7) &&
!"TESTING".startsWith("NG", 0xFFFF) &&
!"TESTING".startsWith("GG") &&
!"TESTING".startsWith("DESTING") &&
!"TESTING".startsWith("TEST", 1) &&
"日本語".endsWith("本語") &&
"日本語".endsWith("日本語") &&
"日本語".endsWith("本", 2) &&
"日本語".endsWith("語", 3);
