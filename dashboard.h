#ifndef DASHBOARD_H
#define DASHBOARD_H

// Initialize and start the ncurses dashboard
// Returns 0 on success, -1 on error
int dashboard_init(void);

// Cleanup and close the dashboard
void dashboard_cleanup(void);

// Update the dashboard display (call this periodically)
void dashboard_update(void);

// Check if dashboard is active
int dashboard_is_active(void);

#endif



