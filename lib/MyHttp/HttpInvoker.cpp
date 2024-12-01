
void
Client::Connection::do_resolve(std::string_view host, std::string_view port)
{
  auto resolverPtr =
    std::make_shared<ba::ip::tcp::resolver>(mStream.get_executor());
  resolverPtr->async_resolve(
    host, port, [this, resolverPtr](auto&& a, auto&& b) { on_resolve(a, b); });
}

void
Client::Connection::on_resolve(
  const BoostEC& ec,
  const ba::ip::tcp::resolver::results_type& results)
{
  if (ec) {
    BOOST_LOG_SEV(mLogger, warn) << "resolve failed: " << ec.message();
    return;
  }

  ba::async_connect(
    mStream.next_layer(),
    results.begin(),
    results.end(),
    [this, resolverPtr](auto&& a, auto&& b) { on_connect(a, b); });
}
