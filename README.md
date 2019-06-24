<hr />
<div align="center">
    <img src="images/logo.svg" alt="Logo" width='300px' height='auto'/>
</div>
<hr />

[![Build status](https://ci.appveyor.com/api/projects/status/wqi79tfettq2t40p/branch/v18?svg=true)](https://ci.appveyor.com/project/albertphil/cga-noder)


### What is CGA?


---

CGA's goal is to become _"a global currency with instantaneous transactions and zero fees over a secure, decentralized network."_

We've applied the philosophy of _"do one thing and do it well."_ We are focused on building the best medium for value exchange in the world.

---

### Key Features

For more information, see [CGACoin.com](https://cgaio.com/) or read the [whitepaper](https://cgaio.com/en/whitepaper).


### Links & Resources

* [CGA Website](https://cgaio.com)




### Setting


sudo ln -s /home/hanvisuser/cga-node/bin/cga_node /usr/local/sbin/cga_node


```

sudo touch /etc/systemd/system/cga_node.service
sudo chmod 664 /etc/systemd/system/cga_node.service
sudo nano /etc/systemd/system/cga_node.service

```

```

[Unit]
Description=CGA node service
After=network.target

[Service]
ExecStart=/home/hanvisuser/cga-node/bin/cga_node --daemon
LimitNOFILE=65536
Restart=on-failure
User=hanvisuser
Group=hanvisuser

[Install]
WantedBy=multi-user.target

```


```

sudo service cga_node start
sudo systemctl enable cga_node

```