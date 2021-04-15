

class WebPage
{
public:
	WebPage(uint port=80);
	~WebPage();

	void Start(void);
	void Loop(void);

private:
	WiFiServer *server_;
};