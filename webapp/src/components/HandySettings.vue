<template>
    <div class="card">
        <div class="card-header">
            <div class="card-title">
                <i class="fas fa-hand-paper"></i>
                Handy
            </div>
            <span class="status-badge" :class="handyStatusClass">{{ handyStatusText }}</span>
        </div>

        <p class="switch-description">
            Connect a <a href="https://www.handyfeeling.com" target="_blank" rel="noopener noreferrer">Handy</a>.
            Only the Connection Key from the mobile app is needed. (There's a separate "Application ID/Key"
            system for a different, developer-account-gated API - not required here.)
        </p>

        <div class="form-group">
            <label for="handyConnectionKey">Connection Key</label>
            <input id="handyConnectionKey" type="text" class="form-input" autocomplete="off"
                   v-model="config.connectionKey" placeholder="Shown in the Handy app">
        </div>

        <div class="config-actions">
            <button @click="saveHandyConfig" class="btn btn-primary" :disabled="savingHandy">
                <div v-if="savingHandy" class="spinner"></div>
                <i v-else class="fas fa-plug"></i>
                {{ savingHandy ? 'Connecting...' : 'Save & Connect' }}
            </button>
        </div>

        <div class="handy-zone-section">
            <label class="switch-label">
                <input type="checkbox" class="switch-input" v-model="config.zone.enabled">
                <span class="switch-slider"></span>
                <span class="switch-text">Randomize movement zone</span>
            </label>
            <p class="switch-description">
                Every interval, picks a new random band within your outer range and moves the
                physical stroke zone there, so the range being used shifts around over time
                instead of staying fixed.
            </p>

            <template v-if="config.zone.enabled">
                <SliderInput id="handyZoneOuterMin" v-model="config.zone.outerMin"
                             title="The lowest position (% of full travel) the zone is ever allowed to use"
                             :min="0" min-title="Bottom"
                             :max="100" max-title="Top" unit="%">
                    <template v-slot:label>
                        Zone range - minimum
                    </template>
                </SliderInput>

                <SliderInput id="handyZoneOuterMax" v-model="config.zone.outerMax"
                             title="The highest position (% of full travel) the zone is ever allowed to use"
                             :min="0" min-title="Bottom"
                             :max="100" max-title="Top" unit="%">
                    <template v-slot:label>
                        Zone range - maximum
                    </template>
                </SliderInput>

                <SliderInput id="handyZoneWidth" v-model="config.zone.subZoneWidth"
                             title="How wide the actively-used band is within the outer range - smaller means more dramatic shifting"
                             :min="5" min-title="Narrow"
                             :max="100" max-title="Full range" unit="%">
                    <template v-slot:label>
                        Active band width
                    </template>
                </SliderInput>

                <SliderInput id="handyZoneInterval" v-model="config.zone.intervalSeconds" unit="s"
                             title="How often a new random position is picked"
                             :min="3" min-title="Frequent"
                             :max="120" max-title="Rare">
                    <template v-slot:label>
                        Shift interval
                    </template>
                </SliderInput>

                <SliderInput id="handyZonePause" v-model="config.zone.pauseSeconds" unit="s"
                             title="Briefly pauses movement before applying each shift - Handy may only pick up a new zone when movement (re)starts"
                             :min="0" min-title="None"
                             :max="10" max-title="Longer">
                    <template v-slot:label>
                        Pause before shift
                    </template>
                </SliderInput>
            </template>
        </div>
    </div>
</template>

<script>
import SliderInput from "@/components/SliderInput.vue";

export default {
    name: 'HandySettings',
    components: {SliderInput},
    data() {
        return {
            savingHandy: false,
            config: {
                connectionKey: '',
                zone: {
                    enabled: false,
                    outerMin: 0,
                    outerMax: 100,
                    subZoneWidth: 40,
                    intervalSeconds: 15,
                    pauseSeconds: 1
                }
            },
            handyStatus: {
                configured: false,
                connected: false
            }
        }
    },
    computed: {
        handyStatusText() {
            if (this.handyStatus.connected) {
                return 'Connected';
            }
            return this.handyStatus.configured ? 'Not Connected' : 'Not Configured';
        },
        handyStatusClass() {
            if (this.handyStatus.connected) {
                return 'bg-success';
            }
            return this.handyStatus.configured ? 'bg-danger' : 'bg-secondary';
        }
    },
    mounted() {
        this.fetchHandyConfig();
    },
    methods: {
        async fetchHandyConfig() {
            try {
                const response = await fetch('/api/handy/config');
                if (!response.ok) {
                    throw new Error('Failed to fetch Handy configuration');
                }

                const data = await response.json();

                this.config = {
                    connectionKey: data.connectionKey || '',
                    zone: {...this.config.zone, ...(data.zone || {})}
                };
                this.handyStatus = {
                    configured: !!data.configured,
                    connected: !!data.connected
                };
            } catch (error) {
                this.$emit('send-notification', 'Error fetching Handy configuration', 'error');
            }
        },
        async saveHandyConfig() {
            this.savingHandy = true;
            try {
                const response = await fetch('/api/handy/config', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json'
                    },
                    body: JSON.stringify({
                        connectionKey: this.config.connectionKey,
                        zone: this.config.zone
                    })
                });

                if (!response.ok) {
                    throw new Error('Failed to save Handy configuration');
                }

                const result = await response.json();
                this.handyStatus = {
                    configured: !!this.config.connectionKey,
                    connected: !!result.connected
                };

                this.$emit('send-notification', result.message || 'Handy configuration saved', result.connected ? 'info' : 'error');
            } catch (error) {
                this.$emit('send-notification', 'Failed to save Handy configuration', 'error');
            } finally {
                this.savingHandy = false;
            }
        }
    }
}
</script>

<style scoped>
.switch-description {
    color: var(--text-secondary);
    font-size: 0.875rem;
    margin: 0 0 1.5rem;
}

.switch-description a {
    color: inherit;
}

.handy-zone-section {
    margin-top: 2rem;
    padding-top: 1.5rem;
    border-top: 1px solid var(--border);
}

.form-group {
    margin-bottom: 1rem;
}

.form-group label {
    display: block;
    margin-bottom: 0.375rem;
    color: var(--text-primary);
    font-weight: 500;
    font-size: 0.875rem;
}

.form-input {
    width: 100%;
    padding: 0.625rem 0.875rem;
    background: var(--surface);
    border: 1px solid var(--border);
    border-radius: var(--border-radius-sm);
    color: var(--text-primary);
    font-size: 0.9375rem;
}

.form-input:focus {
    outline: none;
    border-color: transparent;
    box-shadow: 0 0 0 2px rgba(102, 126, 234, 0.4);
}

.config-actions {
    display: flex;
    gap: 1rem;
    margin: 1.5rem 0;
}

@media (max-width: 768px) {
    .config-actions {
        flex-direction: column;
    }
}
</style>
