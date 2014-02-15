describe("String", function() {
  it("endsWith", function() {
    expect("TESTING".endsWith("ING")).toBe(true);
    expect("TESTING".endsWith("TEST", 4)).toBe(true);
    expect("TESTING".endsWith("NG", 7)).toBe(true);
    expect("TESTING".startsWith("NG", 0xFFFF)).toBe(false);
    expect("TESTING".startsWith("GG")).toBe(false);
    expect("TESTING".startsWith("DESTING")).toBe(false);
    expect("TESTING".startsWith("TEST", 1)).toBe(false);
    expect("日本語".endsWith("本語")).toBe(true);
    expect("日本語".endsWith("日本語")).toBe(true);
    expect("日本語".endsWith("本", 2)).toBe(true);
    expect("日本語".endsWith("語", 3)).toBe(true);
  });
});
