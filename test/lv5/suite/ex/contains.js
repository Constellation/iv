"TESTING".contains("TEST") &&
"TESTING".contains("ING", 4) &&
"TESTING".contains("NG", 5) &&
"TESTING".contains("G", 5) &&
"TESTING".contains("G", 4) &&
"TESTING".contains("NG", 4) &&
"TESTING".contains("EST", 1) &&
!"TESTING".contains("NG", 0xFFFF) &&
!"TESTING".contains("TT") &&
!"TESTING".contains("TESTIND") &&
!"TESTING".contains("ESTT", 1) &&
"日本語".contains("日本") &&
"日本語".contains("日本語", 0) &&
"日本語".contains("本", 1) &&
"日本語".contains("本語", 1) &&
"日本語".contains("語", 2) &&
!"日本語".contains("語", 3) &&
!"日本語".contains("本", 2) &&
!"日本語".contains("日", 1) &&
!"日本語".contains("本語", 2);
