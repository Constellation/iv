describe("arithmetic", function() {
  it("mod -0", function() {
    expect(1 / (-1 % -1)).toBe(Number.NEGATIVE_INFINITY);
  });

  it("mod by 0", function() {
    expect(isNaN(1 % 0)).toBe(true);
  });
});
