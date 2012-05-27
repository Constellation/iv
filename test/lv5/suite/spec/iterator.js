describe("iterator", function() {
  it("for in to String", function() {
    var i = "testing";
    var res = [];
    for (var k in i) {
      res.push(k, i[k]);
    }
    expect(res).toEqual(['0', 't', '1', 'e', '2', 's', '3', 't', '4', 'i', '5', 'n', '6', 'g']);
  });
});
