describe("String", function() {
  it("startsWith", function() {
    expect("TESTING".startsWith("TEST")).toBe(true);
    expect("TESTING".startsWith("ING", 4)).toBe(true);
    expect("TESTING".startsWith("NG", 5)).toBe(true);
    expect("TESTING".startsWith("NG", 0xFFFF)).toBe(false);
    expect("TESTING".startsWith("TT")).toBe(false);
    expect("TESTING".startsWith("TESTIND")).toBe(false);
    expect("TESTING".startsWith("ESTT", 1)).toBe(false);
    expect("日本語".startsWith("日本")).toBe(true);
    expect("日本語".startsWith("日本語", 0)).toBe(true);
    expect("日本語".startsWith("本", 1)).toBe(true);
    expect("日本語".startsWith("語", 2)).toBe(true);
  });
});
