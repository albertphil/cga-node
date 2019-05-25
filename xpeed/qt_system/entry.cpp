#include <xpeed/node/testing.hpp>
#include <xpeed/qt/qt.hpp>

#include <thread>

int main (int argc, char ** argv)
{
	QApplication application (argc, argv);
	QCoreApplication::setOrganizationName ("Xpeed");
	QCoreApplication::setOrganizationDomain ("XpeedCoin.com");
	QCoreApplication::setApplicationName ("Xpeed Wallet");
	xpd_qt::eventloop_processor processor;
	static int count (16);
	xpeed::system system (24000, count);
	std::unique_ptr<QTabWidget> client_tabs (new QTabWidget);
	std::vector<std::unique_ptr<xpd_qt::wallet>> guis;
	for (auto i (0); i < count; ++i)
	{
		xpeed::uint256_union wallet_id;
		xpeed::random_pool::generate_block (wallet_id.bytes.data (), wallet_id.bytes.size ());
		auto wallet (system.nodes[i]->wallets.create (wallet_id));
		xpeed::keypair key;
		wallet->insert_adhoc (key.prv);
		guis.push_back (std::unique_ptr<xpd_qt::wallet> (new xpd_qt::wallet (application, processor, *system.nodes[i], wallet, key.pub)));
		client_tabs->addTab (guis.back ()->client_window, boost::str (boost::format ("Wallet %1%") % i).c_str ());
	}
	client_tabs->show ();
	xpeed::thread_runner runner (system.io_ctx, system.nodes[0]->config.io_threads);
	QObject::connect (&application, &QApplication::aboutToQuit, [&]() {
		system.stop ();
	});
	int result;
	try
	{
		result = application.exec ();
	}
	catch (...)
	{
		result = -1;
		assert (false);
	}
	runner.join ();
	return result;
}
