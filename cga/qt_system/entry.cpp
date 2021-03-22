#include <cga/node/testing.hpp>
#include <cga/qt/qt.hpp>

#include <thread>

int main (int argc, char ** argv)
{
	QApplication application (argc, argv);
	QCoreApplication::setOrganizationName ("CGA");
	QCoreApplication::setOrganizationDomain ("CGACoin.com");
	QCoreApplication::setApplicationName ("CGA Wallet");
	cga_qt::eventloop_processor processor;
	static int count (16);
	cga::system system (24000, count);
	std::unique_ptr<QTabWidget> client_tabs (new QTabWidget);
	std::vector<std::unique_ptr<cga_qt::wallet>> guis;
	for (auto i (0); i < count; ++i)
	{
		cga::uint256_union wallet_id;
		cga::random_pool::generate_block (wallet_id.bytes.data (), wallet_id.bytes.size ());
		auto wallet (system.nodes[i]->wallets.create (wallet_id));
		cga::keypair key;
		wallet->insert_adhoc (key.prv);
		guis.push_back (std::unique_ptr<cga_qt::wallet> (new cga_qt::wallet (application, processor, *system.nodes[i], wallet, key.pub)));
		client_tabs->addTab (guis.back ()->client_window, boost::str (boost::format ("Wallet %1%") % i).c_str ());
	}
	client_tabs->show ();
	cga::thread_runner runner (system.io_ctx, system.nodes[0]->config.io_threads);
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
