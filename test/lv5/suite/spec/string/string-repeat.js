describe("String", function() {
  it("repeat", function() {
    expect("TEST".repeat(2)).toBe("TESTTEST");
    expect("".repeat(1000000)).toBe("");
    expect("T".repeat(10)).toBe("TTTTTTTTTT");
    expect("T".repeat(0)).toBe("");
    expect("T".repeat(-2)).toBe("");
  });
});
