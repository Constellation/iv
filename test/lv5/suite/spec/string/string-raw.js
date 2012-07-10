describe("String", function() {
  it("raw", function() {
    expect(String.raw({
      raw: ['This engine is ', '.']
    }, 'lv5')).toBe('This engine is lv5.');

    expect(String.raw({
      raw: ['This engine is ', '.']
    })).toBe('This engine is undefined.');
  });

  it("raw empty string", function() {
    expect(String.raw({
      raw: []
    })).toBe('');
  });
});
