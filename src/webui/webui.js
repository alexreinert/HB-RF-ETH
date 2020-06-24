Vue.use(BootstrapVue);
Vue.use(window.vuelidate.default);
Vue.use(VueI18n);

var required = validators.required;
var requiredIf = validators.requiredIf;
var requiredUnless = validators.requiredUnless;
var minLength = validators.minLength;
var maxLength = validators.maxLength;
var ipAddress = validators.ipAddress;
var sameAs = validators.sameAs;
var numeric = validators.numeric;

var app = new Vue({
    el: '#app',
    data: {
        login: {
            isLoggedIn: false,
            password: "",
            error: null
        },
        settings: {
            adminPassword: "",
            adminPasswordRepeat: "",
            hostname: "",
            useDHCP: true,
            localIP: "",
            netmask: "",
            gateway: "",
            dns1: "",
            dns2: "",
            timesource: 0,
            dcfOffset: 0,
            gpsBaudrate: 9600,
            ntpServer: "",

            success: null,
            error: null,
        },
        firmware: {
            file: null,
            progress: 0,
            success: null,
            error: null,
        },
        sysInfo: {
            currentVersion: "",
            latestVersion: ""
        }
    },
    validations: {
        login: {
            password: {
                required
            }
        },
        settings: {
            adminPassword: {
                minLength: minLength(5),
                maxLength: maxLength(32),
            },
            adminPasswordRepeat: {
                sameAsPassword: sameAs('adminPassword'),
            },
            hostname: {
                required,
                maxLength: maxLength(32)
            },
            localIP: {
                required: requiredUnless('useDHCP'),
                ipAddress
            },
            netmask: {
                required: requiredUnless('useDHCP'),
                ipAddress
            },
            gateway: {
                required: requiredUnless('useDHCP'),
                ipAddress
            },
            dns1: {
                required: requiredUnless('useDHCP'),
                ipAddress
            },
            dns2: {
                ipAddress
            },
            ntpServer: {
                required: requiredIf('isNtpActived'),
            },
            dcfOffset: {
                required: requiredIf('isDcfActived'),
                numeric
            }
        }
    },
    i18n: {
        locale: navigator.language,
        fallbackLocale: "en",
        messages: {
            de: {
                login: {
                    title: "Bitte anmelden",
                    password: "Passwort",
                    login: "Anmelden",
                    loginError: "Anmelden war nicht erfolgreich."
                },
                settings: {
                    title: "Einstellungen",
                    changePassword: "Passwort ändern",
                    repeatPassword: "Passwort wiederholen",
                    hostname: "Hostname",
                    dhcp: "DHCP",
                    enabled: "Aktiv",
                    disabled: "Deaktiviert",
                    ipAddress: "IP Adresse",
                    netmask: "Netzmaske",
                    gateway: "Gateway",
                    dns1: "Primärer DNS Server",
                    dns2: "Sekundärer DNS Server",
                    timesource: "Zeitquelle",
                    ntp: "NTP",
                    dcf: "DCF",
                    gps: "GPS",
                    ntpServer: "NTP Server",
                    dcfOffset: "DCF Versatz",
                    gpsBaudrate: "GPS Baudrate",
                    save: "Speichern",
                    saveSuccess: "Einstellungen wurden erfolgreich gespeichert. Bitte starten Sie das System neu um sie zu übernehmen.",
                    saveError: "Es ist ein Fehler aufgetreten."
                },
                firmware: {
                    title: "Firmware",
                    installedVersion: "Installierte Version",
                    updateAvailable: "Ein Update auf Version {latestVersion} ist auf {github} verfügbar.",
                    updateFile: "Firmware Datei",
                    noFileChosen: "Keine Datei ausgewählt",
                    browse: "Datei auswählen",
                    upload: "Hochladen",
                    uploadSuccess: "Die Firmware wurde erfolgreich hochgeladen. Bitte starten Sie das System neu um sie zu aktivieren.",
                    uploadError: "Es ist ein Fehler aufgetreten."
                },
                about: {
                    link: "Über"
                }
            },
            en: {
                login: {
                    title: "Please log in",
                    password: "Password",
                    login: "Login",
                    loginError: "Login was not successful."
                },
                settings: {
                    title: "Settings",
                    changePassword: "Change Password",
                    repeatPassword: "Repeat Password",
                    hostname: "Hostname",
                    dhcp: "DHCP",
                    enabled: "Enabled",
                    disabled: "Disabled",
                    ipAddress: "IP address",
                    netmask: "Netmask",
                    gateway: "Gateway",
                    dns1: "Primary DNS",
                    dns2: "Secondary DNS",
                    timesource: "Timesource",
                    ntp: "NTP",
                    dcf: "DCF",
                    gps: "GPS",
                    ntpServer: "NTP Server",
                    dcfOffset: "DCF Offset",
                    gpsBaudrate: "GPS Baudrate",
                    save: "Save",
                    saveSuccess: "Settings were successfully saved. Please restart to take them effect.",
                    saveError: "An error occured."
                },
                firmware: {
                    title: "Firmware",
                    installedVersion: "Installed version",
                    updateAvailable: "An update to version {latestVersion} is available at {github}.",
                    updateFile: "Firmware file",
                    noFileChosen: "No file chosen",
                    browse: "Browse",
                    upload: "Upload",
                    uploadSuccess: "Firmware Update successfully uploaded. Please restart to activate.",
                    uploadError: "An error occured."
                },
                about: {
                    link: "About"
                }
            }
        }
    },
    computed: {
        isNtpActivated: function () {
            return this.settings.timesource == 0;
        },
        isDcfActivated: function () {
            return this.settings.timesource == 1;
        },
        isGpsActivated: function () {
            return this.settings.timesource == 2;
        }
    },
    methods: {
        validateState(group, name) {
            const { $dirty, $error } = this.$v[group][name];
            return $dirty && $error ? false : null;
        },
        loginClick: function (event) {
            var self = this;
            self.login.error = null;
            $.ajax({
                url: "getData.json",
                type: 'GET',
                cache: false,
                dataType: "json",
                headers: { "Authorization": "Basic " + btoa("admin:" + self.login.password) },
                success: function (json) {
                    if (json.loggedIn) {
                        self.login.isLoggedIn = true;
                        self.sysInfo = json.sysInfo;
                        self.settings = json.settings;
                        self.$v.settings.$touch();
                    } else {
                        self.login.error = self.$t('login.loginError');
                    }
                },
                error: function (jqXHR, textStatus, errorThrown) {
                    self.login.error = self.$t('login.loginError');
                }
            });
        },
        firmwareUpdateClick: function (event) {
            var self = this;

            self.firmware.success = null;
            self.firmware.error = null;
            self.firmware.progress = 0;

            var form = new FormData();
            form.append("file", self.firmware.file, self.firmware.file.name);
            form.append("upload_file", true);

            $.ajax({
                type: "POST",
                url: "/ota_update",
                xhr: function () {
                    var myXhr = $.ajaxSettings.xhr();
                    if (myXhr.upload) {
                        myXhr.upload.addEventListener('progress',
                            function (event) {
                                var pos = event.loaded || event.position;
                                var tot = event.total;
                                if (event.lengthComputable) {
                                    self.firmware.progress = Math.ceil(pos / tot * 100);
                                }
                            },
                            false);
                    }
                    return myXhr;
                },
                headers: { "Authorization": "Basic " + btoa("admin:" + self.login.password) },
                success: function (data) {
                    self.firmware.success = self.$t('firmware.uploadSuccess');;
                    self.firmware.progress = 0;
                    self.firmware.file = null;
                },
                error: function (jqXHR, textStatus, errorThrown) {
                    self.firmware.progress = 0;
                    self.firmware.error = self.$t('firmware.uploadError');
                },
                async: true,
                data: form,
                cache: false,
                contentType: false,
                processData: false,
                timeout: 60000
            });
        },
        saveSettingsClick: function (event) {
            var self = this;

            this.$v.settings.$touch();
            if (this.$v.settings.$anyError) return;

            self.settings.error = null;
            self.settings.success = null;

            $.ajax({
                url: "settings.json",
                type: 'POST',
                cache: false,
                dataType: "json",
                data: JSON.stringify(self.settings),
                headers: { "Authorization": "Basic " + btoa("admin:" + self.login.password) },
                success: function (json) {
                    self.settings = json.settings;
                    self.settings.success = self.$t('settings.saveSuccess');
                },
                error: function (jqXHR, textStatus, errorThrown) {
                    self.settings.error = self.$t('settings.saveError');
                },
                complete: function (jqXHR, textStatus) {
                    self.settings.adminPasswordRepeat = "";
                    self.settings.adminPassword = "";
                }
            });
        }
    }
});
