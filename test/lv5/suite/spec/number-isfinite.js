describe("Number", function() {
  it("isFinite", function() {
    expect(Number.isFinite(NaN)).toBe(false);
    expect(Number.isFinite(10)).toBe(true);
    expect(Number.isFinite(-20)).toBe(true);
    expect(Number.isFinite(222.222)).toBe(true);
    expect(Number.isFinite(-222.222)).toBe(true);
    expect(Number.isFinite(0)).toBe(true);
    expect(Number.isFinite(-0)).toBe(true);
    expect(Number.isFinite(Infinity)).toBe(false);
    expect(Number.isFinite(-Infinity)).toBe(false);
    expect(Number.isFinite("0")).toBe(false);
    expect(Number.isFinite(/test/)).toBe(false);
    expect(Number.isFinite({})).toBe(false);
    expect(Number.isFinite([])).toBe(false);
    expect(Number.isFinite(function() { })).toBe(false);
  });
});
