<template>
    <div class="app-container">
        <div class="header">
            <h1>
                <img src="/favicon.svg" alt="NogasmLink" height="40" width="40">
                NogasmLink (for the Handy)
            </h1>
            <p>Intelligent Arousal Management System</p>
        </div>

        <div class="dashboard">
            <div class="card">
                <div class="card-header">
                    <div class="card-title">
                        <i class="fas fa-wifi"></i>
                        Status
                    </div>
                    <div class="status-indicator" :class="websocketService.getWebSocketStatusClass()">
                        <i class="fas fa-circle"></i>
                        {{ websocketService.formatWebSocketStatus() }}
                    </div>
                </div>

                <div class="metric-grid">
                    <div class="metric">
                        <div class="metric-value">{{ status.wifi.connected ? formatSignalStrength(status.wifi.rssi) : 'Disconnected' }}</div>
                        <div class="metric-label">WiFi Signal</div>
                    </div>
                    <div v-if="status.handy.configured" class="metric">
                        <div class="metric-value">
                            <span class="status-badge" :class="status.handy.connected ? 'bg-success' : 'bg-danger'">
                                {{ status.handy.connected ? 'CONNECTED' : 'DISCONNECTED' }}
                            </span>
                        </div>
                        <div class="metric-label">Handy Status</div>
                    </div>
                </div>

                <div v-if="status.wifi.connected" class="connection-info">
                    <p class="text-secondary"><strong>Network: </strong>{{ status.wifi.ssid }}</p>
                    <p class="text-secondary"><strong>IP Address: </strong>{{ status.wifi.ip }}</p>
                </div>
            </div>

            <handy-settings @send-notification="notify"/>

            <arousal-control class="full-width" @send-notification="notify"/>
            <arousal-tracker class="full-width" @send-notification="notify"/>
        </div>

        <div class="notifications">
            <div v-for="notification in notifications" :key="notification.id"
                 class="notification" :class="notification.type">
                <i class="fas fa-circle-info"></i>
                {{ notification.text }}
            </div>
        </div>
    </div>
</template>

<style>
@import './assets/nogasmlink-theme.css';
</style>

<script>
import HandySettings from './components/HandySettings.vue'
import ArousalControl from './components/ArousalControl.vue'
import ArousalTracker from './components/ArousalTracker.vue'
import websocketService from './services/WebSocketService.js'

export default {
    name: 'App',
    components: {
        HandySettings,
        ArousalControl,
        ArousalTracker
    },
    data() {
        return {
            status: {
                wifi: {
                    connected: false,
                    ssid: '',
                    ip: '',
                    rssi: -1
                },
                handy: {
                    configured: false,
                    connected: false
                }
            },
            lastNotificationId: 0,
            notifications: [],
            unsubscribeFunctions: [],
            websocketService
        }
    },
    mounted() {
        this.initWebSocket();
        this.fetchStatus();
    },
    beforeUnmount() {
        this.unsubscribeFunctions.forEach(unsubscribe => unsubscribe());
    },
    methods: {
        initWebSocket() {
            websocketService.connect().catch(() => {
                this.notify('Failed to connect to WebSocket', 'error');
            });

            this.unsubscribeFunctions.push(
                websocketService.subscribe('ble_status', (data) => {
                    this.updateHandyStatus(data);
                })
            );
        },

        formatSignalStrength(strength) {
            strength = Math.abs(strength);
            if (strength < 70) {
                return 'Excellent';
            } else if (strength >= 70 && strength < 85) {
                return 'Good';
            } else if (strength >= 85 && strength < 100) {
                return 'Fair';
            } else if (strength >= 100 && strength < 115) {
                return 'Poor';
            } else {
                return 'Bad signal (' + strength + ')';
            }
        },

        notify(message, type = 'info') {
            this.notifications.push({
                id: ++this.lastNotificationId,
                text: message,
                type: type
            });

            const lastId = this.lastNotificationId;
            setTimeout(() => {
                const index = this.notifications.map(n => n.id).indexOf(lastId);
                this.notifications.splice(index, 1);
            }, 4000);
        },

        updateHandyStatus(data) {
            this.status.wifi.rssi = data.wifi.rssi;

            if (data.handy) {
                this.status.handy.configured = data.handy.configured;
                this.status.handy.connected = data.handy.connected;
            }
        },

        async fetchStatus() {
            try {
                const response = await fetch('/api/status');
                if (!response.ok) {
                    throw new Error('Failed to fetch status');
                }

                const data = await response.json();

                this.status.wifi = data.wifi;

                if (data.ble?.handy) {
                    this.status.handy = data.ble.handy;
                }
            } catch (error) {
                this.notify('Error fetching status', 'error');
            }
        }
    }
}
</script>
