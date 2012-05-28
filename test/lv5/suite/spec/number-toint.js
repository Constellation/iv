describe("Number", function() {
  it("toInt", function() {
    expect(Number.toInt(NaN)).toBe(0);
    expect(Number.toInt(10)).toBe(10);
    expect(Number.toInt(-20)).toBe(-20);
    expect(Number.toInt(222.222)).toBe(222);
    expect(Number.toInt(-222.222)).toBe(-222);
    expect(Number.toInt(0)).toBe(0);
    expect(1 / Number.toInt(0)).toBe(Infinity);
    expect(Number.toInt(-0)).toBe(-0);
    expect(1 / Number.toInt(-0)).toBe(-Infinity);
    expect(Number.toInt(Infinity)).toBe(Infinity);
    expect(Number.toInt(-Infinity)).toBe(-Infinity);
    expect(Number.toInt("0")).toBe(0);
    expect(1 / Number.toInt("0")).toBe(Infinity);
    expect(Number.toInt("-0")).toBe(0);
    expect(1 / Number.toInt("-0")).toBe(-Infinity);
    expect(Number.toInt("t")).toBe(0);
    expect(1 / Number.toInt("t")).toBe(Infinity);
    expect(Number.toInt(/test/)).toBe(0);
    expect(1 / Number.toInt(/test/)).toBe(Infinity);
    expect(Number.toInt({})).toBe(0);
    expect(1 / Number.toInt({})).toBe(Infinity);
    expect(Number.toInt([])).toBe(0);
    expect(1 / Number.toInt([])).toBe(Infinity);
    expect(Number.toInt(function() { })).toBe(0);
    expect(1 / Number.toInt(function() { })).toBe(Infinity);
  });
});
