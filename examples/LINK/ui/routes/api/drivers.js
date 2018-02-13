
export function get(req, res, next) {
  const drivers = ['MSSQL','Oracle'];
  res.json(drivers);
}
