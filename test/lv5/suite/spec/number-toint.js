describe("Number", function() {
  it("toInteger", function() {
    expect(Number.toInteger(NaN)).toBe(0);
    expect(Number.toInteger(10)).toBe(10);
    expect(Number.toInteger(-20)).toBe(-20);
    expect(Number.toInteger(222.222)).toBe(222);
    expect(Number.toInteger(-222.222)).toBe(-222);
    expect(Number.toInteger(0)).toBe(0);
    expect(1 / Number.toInteger(0)).toBe(Infinity);
    expect(Number.toInteger(-0)).toBe(-0);
    expect(1 / Number.toInteger(-0)).toBe(-Infinity);
    expect(Number.toInteger(Infinity)).toBe(Infinity);
    expect(Number.toInteger(-Infinity)).toBe(-Infinity);
    expect(Number.toInteger("0")).toBe(0);
    expect(1 / Number.toInteger("0")).toBe(Infinity);
    expect(Number.toInteger("-0")).toBe(0);
    expect(1 / Number.toInteger("-0")).toBe(-Infinity);
    expect(Number.toInteger("t")).toBe(0);
    expect(1 / Number.toInteger("t")).toBe(Infinity);
    expect(Number.toInteger(/test/)).toBe(0);
    expect(1 / Number.toInteger(/test/)).toBe(Infinity);
    expect(Number.toInteger({})).toBe(0);
    expect(1 / Number.toInteger({})).toBe(Infinity);
    expect(Number.toInteger([])).toBe(0);
    expect(1 / Number.toInteger([])).toBe(Infinity);
    expect(Number.toInteger(function() { })).toBe(0);
    expect(1 / Number.toInteger(function() { })).toBe(Infinity);
  });
});
