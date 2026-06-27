using namespace System.Security.Cryptography
using namespace System.Security.Cryptography.X509Certificates

[Threading.Thread]::CurrentThread.CurrentUICulture = 'en-US'

Add-Type -AssemblyName netstandard
Add-Type -AssemblyName System.Security.Cryptography

$my_store = New-Object -TypeName X509Store -ArgumentList ([StoreName]::My, [StoreLocation]::LocalMachine)
$my_store.Open([OpenFlags]::ReadWrite)
$existing = $my_store.Certificates.Find([X509FindType]::FindBySubjectName,"lesserjoy",$false)
if($existing.Count -gt 0) {
    "Cert already generated!"
    exit
}

$rsa = [RSA]::Create(2048);
$req = New-Object -TypeName CertificateRequest -ArgumentList ("CN=lesserjoy",$rsa,[HashAlgorithmName]::SHA256, [RSASignaturePadding]::Pkcs1);

$ext = New-Object -TypeName X509KeyUsageExtension -ArgumentList ([X509KeyUsageFlags]::DigitalSignature,$false)
$req.CertificateExtensions.Add($ext);

$oid = New-Object -TypeName Oid -ArgumentList ("1.3.6.1.5.5.7.3.3")
$oid_collection = New-Object -TypeName OidCollection
$oid_collection.Add($oid)
$enhanced_ext = New-Object -TypeName X509EnhancedKeyUsageExtension -ArgumentList ($oid_collection,$false)

$req.CertificateExtensions.Add($enhanced_ext)

$utc = [DateTimeOffset]::UtcNow

$days = $utc.AddDays(-1)
$years= $utc.AddYears(10)

$cert = $req.CreateSelfSigned($days, $years)
$cert.FriendlyName = "lesserjoy"

$pfx = $cert.Export([X509ContentType]::Pfx)
$persistable_cert = [X509CertificateLoader]::LoadPkcs12($pfx, "", [X509KeyStorageFlags].PersistKeySet -bor [X509KeyStorageFlags].MachineKeySet -bor [X509KeyStorageFlags].Exportable)
$persistable_cert.FriendlyName = "lesserjoy"

$my_store.Add($persistable_cert)

$root_store = New-Object -TypeName X509Store -ArgumentList ([StoreName]::Root, [StoreLocation]::LocalMachine)
$root_store.Open([OpenFlags]::ReadWrite)
$root_store.Add($persistable_cert)

$trusted_publisher_store = New-Object -TypeName X509Store -ArgumentList ([StoreName]::TrustedPublisher, [StoreLocation]::LocalMachine)
$trusted_publisher_store.Open([OpenFlags]::ReadWrite)
$trusted_publisher_store.Add($persistable_cert)

"Cert generated!"
