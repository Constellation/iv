describe("String", function() {
  it("codePointAt", function() {
    expect('lv5'.codePointAt(0)).toBe('l'.charCodeAt(0));
    expect('lv5'.codePointAt(1)).toBe('v'.charCodeAt(0));
    expect('lv5'.codePointAt(2)).toBe('5'.charCodeAt(0));
  });

  it("codePointAt with surrogate pair", function() {
    expect('\ud842\udfb7'.codePointAt(0)).toBe(0x20bb7);
    expect('\ud842\udfb7'.codePointAt(1)).toBe(0xdfb7);
  });

  it("codePointAt with invalid surrogate pair", function() {
    expect('\ud842\uf000'.codePointAt(0)).toBe(0xd842);
    expect('\ud7ff\uf000'.codePointAt(0)).toBe(0xd7ff);
    expect('\ud842'.codePointAt(0)).toBe(0xd842);
  });

  it("codePointAt out of range", function() {
    expect(isNaN('\ud842\udfb7'.codePointAt(2))).toBe(true);
    expect(isNaN('\ud842\udfb7'.codePointAt(-2))).toBe(true);
    expect(isNaN('\ud842\udfb7'.codePointAt(2.5))).toBe(true);
  });
});
