describe("String", function() {
  it("contains", function() {
    expect("TESTING".contains("TEST")).toBe(true);
    expect("TESTING".contains("ING", 4)).toBe(true);
    expect("TESTING".contains("NG", 5)).toBe(true);
    expect("TESTING".contains("G", 5)).toBe(true);
    expect("TESTING".contains("G", 4)).toBe(true);
    expect("TESTING".contains("NG", 4)).toBe(true);
    expect("TESTING".contains("EST", 1)).toBe(true);
    expect("TESTING".contains("NG", 0xFFFF)).toBe(false);
    expect("TESTING".contains("TT")).toBe(false);
    expect("TESTING".contains("TESTIND")).toBe(false);
    expect("TESTING".contains("ESTT", 1)).toBe(false);
    expect("日本語".contains("日本")).toBe(true);
    expect("日本語".contains("日本語", 0)).toBe(true);
    expect("日本語".contains("本", 1)).toBe(true);
    expect("日本語".contains("本語", 1)).toBe(true);
    expect("日本語".contains("語", 2)).toBe(true);
    expect("日本語".contains("語", 3)).toBe(false);
    expect("日本語".contains("本", 2)).toBe(false);
    expect("日本語".contains("日", 1)).toBe(false);
    expect("日本語".contains("本語", 2)).toBe(false);
  });
});
