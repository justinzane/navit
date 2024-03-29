#! /usr/bin/python
import dbus
import sys
bus = dbus.SessionBus()
conn = bus.get_object('org.navit_project.navit',
                       '/org/navit_project/navit')
iface = dbus.Interface(conn, dbus_interface='org.navit_project.navit');
iter=iface.iter();
navit=bus.get_object('org.navit_project.navit', conn.get_navit(iter));
iface.iter_destroy(iter);
navit_iface = dbus.Interface(navit, dbus_interface='org.navit_project.navit.navit');
print navit_iface.evaluate(sys.argv[1]);
