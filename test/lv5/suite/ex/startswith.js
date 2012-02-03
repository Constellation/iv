"TESTING".startsWith("TEST") &&
"TESTING".startsWith("ING", 4) &&
"TESTING".startsWith("NG", 5) &&
!"TESTING".startsWith("NG", 0xFFFF) &&
!"TESTING".startsWith("TT") &&
!"TESTING".startsWith("TESTIND") &&
!"TESTING".startsWith("ESTT", 1) &&
"日本語".startsWith("日本") &&
"日本語".startsWith("日本語", 0) &&
"日本語".startsWith("本", 1) &&
"日本語".startsWith("語", 2);
